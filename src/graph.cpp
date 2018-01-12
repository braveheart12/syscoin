#include "graph.h"
#include "offer.h"
#include "cert.h"
#include "alias.h"
#include "asset.h"
#include "assetallocation.h"
using namespace boost;
typedef adjacency_list< vecS, vecS, directedS > Graph;
typedef graph_traits<Graph> Traits;
typedef typename Traits::vertex_descriptor vertex_descriptor;
typedef typename std::vector< vertex_descriptor > container;
typedef typename property_map<Graph, vertex_index_t>::const_type IndexMap;

bool CreateDAGFromBlock(const CBlock*pblock, Graph &graph, std::vector<vertex_descriptor> &vertices, std::unordered_map<int, int> &mapTxIndex) {
	std::unordered_map<string, int> mapAliasIndex;
	std::vector<vector<unsigned char> > vvchArgs;
	std::vector<vector<unsigned char> > vvchAliasArgs;
	int op;
	int nOut;
	for (unsigned int n = 0; n< pblock->vtx.size(); n++) {
		const CTransaction& tx = pblock->vtx[n];
		if (tx.nVersion == SYSCOIN_TX_VERSION)
		{
			if (!DecodeAliasTx(tx, op, nOut, vvchAliasArgs))
				continue;

			if (DecodeAssetAllocationTx(tx, op, nOut, vvchArgs))
			{
				const string& sender = stringFromVch(vvchAliasArgs[0]);
				const unsigned int verticesSize = vertices.size() - 1;
				if (mapAliasIndex.count(sender) == 0) {
					vertices.push_back(add_vertex(graph));
					mapAliasIndex[sender] = verticesSize;
					mapTxIndex[verticesSize] = nOut;
				}
				LogPrintf("CreateDAGFromBlock: found asset allocation from sender %s\n", sender);
				CAssetAllocation allocation(tx);
				if (!allocation.listSendingAllocationAmounts.empty()) {
					for (auto& allocationInstance : allocation.listSendingAllocationAmounts) {
						const string& receiver = stringFromVch(allocationInstance.first);
						if (mapAliasIndex.count(receiver) == 0) {
							vertices.push_back(add_vertex(graph));
							mapAliasIndex[receiver] = verticesSize;
						}
						// the graph needs to be from index to index 
						add_edge(vertices[mapAliasIndex[receiver]], vertices[mapAliasIndex[sender]], graph);
						LogPrintf("CreateDAGFromBlock: add edge from %s(index %d) to %s(index %d)\n", receiver, mapAliasIndex[receiver],  sender, mapAliasIndex[sender]);
					}
				}
			}
		}
	}
	return mapTxIndex.size() > 0;
}
unsigned int DAGRemoveCycles(CBlock * pblock, std::unique_ptr<CBlockTemplate> &pblocktemplate, uint64_t &nBlockTx, uint64_t &nBlockSize, unsigned int &nBlockSigOps, CAmount &nFees) {
	LogPrintf("DAGRemoveCycles\n");
	std::vector<CTransaction> newVtx;
	std::vector<vertex_descriptor> vertices;
	std::unordered_map<int, int> mapTxIndex;
	Graph graph;

	if (!CreateDAGFromBlock(pblock, graph, vertices, mapTxIndex)) {
		return true;
	}

	sorted_vector<int> clearedVertices;
	cycle_visitor<sorted_vector<int> > visitor(clearedVertices);
	hawick_circuits(graph, visitor);
	LogPrintf("Found %d circuits\n", clearedVertices.size());
	// iterate backwards over sorted list of vertices, we can do this because we remove vertices from end to beginning, 
	// which invalidate iterators from positon removed to end (we don't care about those after removal since we are iterating backwards to begining)
	reverse(clearedVertices.begin(), clearedVertices.end());
	for (auto& nVertex : clearedVertices) {
		LogPrintf("trying to clear vertex %d\n", nVertex);
		// mapTxIndex knows of the mapping between vertices and tx vout position, add 1 to account for coinbase (0) vout
		const unsigned int nOut = mapTxIndex[nVertex]+1;
		if (nOut >= pblock->vtx.size())
			continue;
		LogPrintf("cleared vertex, erasing nOut %d\n", nOut);
		const CTransaction& txToRemove = pblock->vtx[nOut];
		nFees -= pblocktemplate->vTxFees[nOut];
		nBlockSigOps -= pblocktemplate->vTxSigOps[nOut];
		nBlockSize -= txToRemove.GetTotalSize();
		pblock->vtx.erase(pblock->vtx.begin() + nOut);
		nBlockTx--;
		pblocktemplate->vTxFees.erase(pblocktemplate->vTxFees.begin() + nOut);
		pblocktemplate->vTxSigOps.erase(pblocktemplate->vTxSigOps.begin() + nOut);
	}
	return clearedVertices.size();
}
bool DAGTopologicalSort(CBlock * pblock) {
	LogPrintf("DAGTopologicalSort\n");
	std::vector<CTransaction> newVtx;
	std::vector<vertex_descriptor> vertices;
	std::unordered_map<int, int> mapTxIndex;
	Graph graph;

	if (!CreateDAGFromBlock(pblock, graph, vertices, mapTxIndex)) {
		return true;
	}

	container c;
	try
	{
		topological_sort(graph, std::back_inserter(c));
	}
	catch (not_a_dag const& e){
		LogPrintf("DAGTopologicalSort: Not a DAG: %s\n", e.what());
		return false;
	}
	const IndexMap &indices = get(vertex_index, (const Graph &)graph);
	// add coinbase
	newVtx.push_back(pblock->vtx[0]);

	// add sys tx's to newVtx in sorted order
	reverse(c.begin(), c.end());
	string ordered = "";
	for (auto& t:c) {
		LogPrintf("add sys tx in sorted order\n");
		if (mapTxIndex.count(t)) {
			const int &nIndex = get(indices, t);
			const int &nOut = mapTxIndex[nIndex];
			LogPrintf("push nOut %d\n", nOut);
			newVtx.push_back(pblock->vtx[nOut]);
			ordered += strprintf("%d ", nOut);
		}
	}
	LogPrintf("topological ordering: %s\n", ordered);
	// add non sys tx's to end of newVtx
	for (unsigned int nOut = 1; nOut< pblock->vtx.size(); nOut++) {
		const CTransaction& tx = pblock->vtx[nOut];
		if (tx.nVersion != SYSCOIN_TX_VERSION)
		{
			newVtx.push_back(pblock->vtx[nOut]);
		}
	}
	LogPrintf("newVtx size %d vs pblock.vtx size %d\n", newVtx.size(), pblock->vtx.size());
	if (pblock->vtx.size() != newVtx.size())
	{
		LogPrintf("DAGTopologicalSort: sorted block transaction count does not match unsorted block transaction count!\n");
		return false;
	}
	// set newVtx to block's vtx so block can process as normal
	pblock->vtx = newVtx;
	return true;
}