#include "test_syscoin_services.h"
#include "utiltime.h"
#include "util.h"
#include "amount.h"
#include "rpc/server.h"
#include "feedback.h"
#include "cert.h"
#include "escrow.h"
#include "offer.h"
#include "alias.h"
#include "wallet/crypter.h"
#include "random.h"
#include "base58.h"
#include "chainparams.h"
#include <memory>
#include <string>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
static int node1LastBlock=0;
static int node2LastBlock=0;
static int node3LastBlock=0;
static int node4LastBlock=0;
static bool node1Online = false;
static bool node2Online = false;
static bool node3Online = false;
// SYSCOIN testing setup
void StartNodes()
{
	printf("Stopping any test nodes that are running...\n");
	StopNodes();
	node1LastBlock=0;
	node2LastBlock=0;
	node3LastBlock=0;
	node4LastBlock=0;
	if(boost::filesystem::exists(boost::filesystem::system_complete("node1/wallet.dat")))
		boost::filesystem::remove(boost::filesystem::system_complete("node1//wallet.dat"));
	if(boost::filesystem::exists(boost::filesystem::system_complete("node2/wallet.dat")))
		boost::filesystem::remove(boost::filesystem::system_complete("node2//wallet.dat"));
	if(boost::filesystem::exists(boost::filesystem::system_complete("node3/wallet.dat")))
		boost::filesystem::remove(boost::filesystem::system_complete("node3//wallet.dat"));
	if(boost::filesystem::exists(boost::filesystem::system_complete("node4/wallet.dat")))
		boost::filesystem::remove(boost::filesystem::system_complete("node4//wallet.dat"));
	StopMainNetNodes();
	printf("Starting 4 nodes in a regtest setup...\n");
	StartNode("node1");
	StartNode("node2");
	StartNode("node3");
	StartNode("node4", true, "-txindex");
	StopNode("node4");
	StartNode("node4", true, "-txindex");
	SelectParams(CBaseChainParams::REGTEST);

}
void StartMainNetNodes()
{
	StopMainNetNodes();
	printf("Starting 1 node in mainnet setup...\n");
	StartNode("mainnet1", false);
}
void StopMainNetNodes()
{
	printf("Stopping mainnet1..\n");
	try{
		CallRPC("mainnet1", "stop");
	}
	catch(const runtime_error& error)
	{
	}	
	printf("Done!\n");
}
void StopNodes()
{
	StopNode("node1");
	StopNode("node2");
	StopNode("node3");
	StopNode("node4");
	printf("Done!\n");
}
void StartNode(const string &dataDir, bool regTest, const string& extraArgs)
{
	if(boost::filesystem::exists(boost::filesystem::system_complete(dataDir + "/wallet.dat")))
	{
		if (!boost::filesystem::exists(boost::filesystem::system_complete(dataDir + "/regtest")))
			boost::filesystem::create_directory(boost::filesystem::system_complete(dataDir + "/regtest"));
		boost::filesystem::copy_file(boost::filesystem::system_complete(dataDir + "/wallet.dat"),boost::filesystem::system_complete(dataDir + "/regtest/wallet.dat"),boost::filesystem::copy_option::overwrite_if_exists);
		boost::filesystem::remove(boost::filesystem::system_complete(dataDir + "/wallet.dat"));
	}
    boost::filesystem::path fpath = boost::filesystem::system_complete("../syscoind");
	string nodePath = fpath.string() + string(" -datadir=") + dataDir;
	if(regTest)
		nodePath += string(" -regtest -debug -addressindex");
	if(!extraArgs.empty())
		nodePath += string(" ") + extraArgs;
	if (!boost::filesystem::exists(boost::filesystem::system_complete(dataDir + "/db")))
		boost::filesystem::create_directory(boost::filesystem::system_complete(dataDir + "/db"));
	if (dataDir == "node1") {
		printf("Launching mongod on port 27017...\n");
		string path = string("mongod --port=27017 --quiet --dbpath=") + boost::filesystem::system_complete(dataDir + string("/db")).string();
		path += string(" --logpath=") + boost::filesystem::system_complete(dataDir + string("/db/log")).string();
		boost::thread t(runCommand, path);
	}
	else if (dataDir == "node2") {
		printf("Launching mongod on port 27018...\n");
		string path = string("mongod --port=27018 --quiet --dbpath=") + boost::filesystem::system_complete(dataDir + string("/db")).string();
		path += string(" --logpath=") + boost::filesystem::system_complete(dataDir + string("/db/log")).string();
		boost::thread t(runCommand, path);
	}
	else if (dataDir == "node3") {
		printf("Launching mongod on port 27019...\n");
		string path = string("mongod --port=27019 --quiet --dbpath=") + boost::filesystem::system_complete(dataDir + string("/db")).string();
		path += string(" --logpath=") + boost::filesystem::system_complete(dataDir + string("/db/log")).string();
		boost::thread t(runCommand, path);
	}
    boost::thread t(runCommand, nodePath);
	printf("Launching %s, waiting 1 second before trying to ping...\n", nodePath.c_str());
	MilliSleep(1000);
	UniValue r;
	while (1)
	{
		try{
			printf("Calling getinfo!\n");
			r = CallRPC(dataDir, "getinfo", regTest);
			if(dataDir == "node1")
			{
				if(node1LastBlock > find_value(r.get_obj(), "blocks").get_int())
				{
					printf("Waiting for %s to catch up, current block number %d vs total blocks %d...\n", dataDir.c_str(), find_value(r.get_obj(), "blocks").get_int(), node1LastBlock);
					MilliSleep(500);
					continue;
				}
				node1Online = true;
				node1LastBlock = 0;
			}
			else if(dataDir == "node2")
			{
				if(node2LastBlock > find_value(r.get_obj(), "blocks").get_int())
				{
					printf("Waiting for %s to catch up, current block number %d vs total blocks %d...\n", dataDir.c_str(), find_value(r.get_obj(), "blocks").get_int(), node2LastBlock);
					MilliSleep(500);
					continue;
				}
				node2Online = true;
				node2LastBlock = 0;
			}
			else if(dataDir == "node3")
			{
				if(node3LastBlock > find_value(r.get_obj(), "blocks").get_int())
				{
					printf("Waiting for %s to catch up, current block number %d vs total blocks %d...\n", dataDir.c_str(), find_value(r.get_obj(), "blocks").get_int(), node3LastBlock);
					MilliSleep(500);
					continue;
				}
				node3Online = true;
				node3LastBlock = 0;
			}
			else if(dataDir == "node4")
			{
				if(node4LastBlock > find_value(r.get_obj(), "blocks").get_int())
				{
					printf("Waiting for %s to catch up, current block number %d vs total blocks %d...\n", dataDir.c_str(), find_value(r.get_obj(), "blocks").get_int(), node4LastBlock);
					MilliSleep(500);
					continue;
				}
				node4LastBlock = 0;
			}
			CallRPC(dataDir, "prunesyscoinservices", regTest);
		}
		catch(const runtime_error& error)
		{
			printf("Waiting for %s to come online, trying again in 1 second...\n", dataDir.c_str());
			MilliSleep(1000);
			continue;
		}
		break;
	}
	printf("Done!\n");
}

void StopNode (const string &dataDir) {
	printf("Stopping %s..\n", dataDir.c_str());
	UniValue r;
	try{
		r = CallRPC(dataDir, "getinfo");
		if(r.isObject())
		{
			if(dataDir == "node1")
				node1LastBlock = find_value(r.get_obj(), "blocks").get_int();
			else if(dataDir == "node2")
				node2LastBlock = find_value(r.get_obj(), "blocks").get_int();
			else if(dataDir == "node3")
				node3LastBlock = find_value(r.get_obj(), "blocks").get_int();
			else if(dataDir == "node4")
				node4LastBlock = find_value(r.get_obj(), "blocks").get_int();
		}
	}
	catch(const runtime_error& error)
	{
	}
	try{
		CallRPC(dataDir, "stop");
	}
	catch(const runtime_error& error)
	{
	}
	while (1)
	{
		try {
			MilliSleep(1000);
			CallRPC(dataDir, "getinfo");
		}
		catch (const runtime_error& error)
		{
			break;
		}
	}
	try {
		if (dataDir == "node1" && node1Online) {
			printf("Stopping mongod on port 27017...\n");
			string path = string("mongod --port=27017 --quiet --shutdown --dbpath=") + boost::filesystem::system_complete(dataDir + string("/db")).string();
			path += string(" --logpath=") + boost::filesystem::system_complete(dataDir + string("/db/log")).string();
			boost::thread t(runCommand, path);
		}
		else if (dataDir == "node2" && node2Online) {
			printf("Stopping mongod on port 27018...\n");
			string path = string("mongod --port=27018 --quiet --shutdown --dbpath=") + boost::filesystem::system_complete(dataDir + string("/db")).string();
			path += string(" --logpath=") + boost::filesystem::system_complete(dataDir + string("/db/log")).string();
			boost::thread t(runCommand, path);
		}
		else if (dataDir == "node3" && node3Online) {
			printf("Stopping mongod on port 27019...\n");
			string path = string("mongod --port=27019 --quiet --shutdown --dbpath=") + boost::filesystem::system_complete(dataDir + string("/db")).string();
			path += string(" --logpath=") + boost::filesystem::system_complete(dataDir + string("/db/log")).string();
			boost::thread t(runCommand, path);
		}
		if (dataDir == "node1")
			node1Online = false;
		else if (dataDir == "node2")
			node2Online = false;
		else if (dataDir == "node3")
			node3Online = false;
	}
	catch (const runtime_error& error)
	{
		
	}
	MilliSleep(1000);
	if(boost::filesystem::exists(boost::filesystem::system_complete(dataDir + "/regtest/wallet.dat")))
		boost::filesystem::copy_file(boost::filesystem::system_complete(dataDir + "/regtest/wallet.dat"),boost::filesystem::system_complete(dataDir + "/wallet.dat"),boost::filesystem::copy_option::overwrite_if_exists);
	if(boost::filesystem::exists(boost::filesystem::system_complete(dataDir + "/regtest")))
		boost::filesystem::remove_all(boost::filesystem::system_complete(dataDir + "/regtest"));
	try {
		if (boost::filesystem::exists(boost::filesystem::system_complete(dataDir + "/db")))
			boost::filesystem::remove_all(boost::filesystem::system_complete(dataDir + "/db"));
	}
	catch (...) {
	}
}

UniValue CallRPC(const string &dataDir, const string& commandWithArgs, bool regTest, bool readJson)
{
	UniValue val;
	boost::filesystem::path fpath = boost::filesystem::system_complete("../syscoin-cli");
	string path = fpath.string() + string(" -datadir=") + dataDir;
	if(regTest)
		path += string(" -regtest ");
	else
		path += " ";
	path += commandWithArgs;
	string rawJson = CallExternal(path);
	if(readJson)
	{
		val.read(rawJson);
		if(val.isNull())
			throw runtime_error("Could not parse rpc results");
	}
	return val;
}
int fsize(FILE *fp){
    int prev=ftell(fp);
    fseek(fp, 0L, SEEK_END);
    int sz=ftell(fp);
    fseek(fp,prev,SEEK_SET); //go back to where we were
    return sz;
}
void safe_fclose(FILE* file)
{
      if (file)
         BOOST_VERIFY(0 == fclose(file));
	if(boost::filesystem::exists("cmdoutput.log"))
		boost::filesystem::remove("cmdoutput.log");
}
int runSysCommand(const std::string& strCommand)
{
    int nErr = ::system(strCommand.c_str());
	return nErr;
}
std::string CallExternal(std::string &cmd)
{
	cmd += " > cmdoutput.log || true";
	if(runSysCommand(cmd))
		return string("ERROR");
    boost::shared_ptr<FILE> pipe(fopen("cmdoutput.log", "r"), safe_fclose);
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
	if(fsize(pipe.get()) > 0)
	{
		while (!feof(pipe.get())) {
			if (fgets(buffer, 128, pipe.get()) != NULL)
				result += buffer;
		}
	}
    return result;
}
void GenerateMainNetBlocks(int nBlocks, const string& node)
{
	int targetHeight, newHeight;
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "getinfo"));
	targetHeight = find_value(r.get_obj(), "blocks").get_int() + nBlocks;
	newHeight = 0;
	const string &sBlocks = strprintf("%d",nBlocks);

	while(newHeight < targetHeight)
	{
	  BOOST_CHECK_NO_THROW(r = CallRPC(node, "generate " + sBlocks));
	  MilliSleep(1000);
	  BOOST_CHECK_NO_THROW(r = CallRPC(node, "getinfo"));
	  newHeight = find_value(r.get_obj(), "blocks").get_int();
	  BOOST_CHECK_NO_THROW(r = CallRPC(node, "getinfo"));
	  CAmount balance = AmountFromValue(find_value(r.get_obj(), "balance"));
	  printf("Current block height %d, Target block height %d, balance %f\n", newHeight, targetHeight, ValueFromAmount(balance).get_real()); 
	}
	BOOST_CHECK(newHeight >= targetHeight);
}
// generate n Blocks, with up to 10 seconds relay time buffer for other nodes to get the blocks.
// may fail if your network is slow or you try to generate too many blocks such that can't relay within 10 seconds
void GenerateBlocks(int nBlocks, const string& node)
{
  int height, newHeight, timeoutCounter;
  UniValue r;
  string otherNode1, otherNode2;
  GetOtherNodes(node, otherNode1, otherNode2);
  try
  {
	r = CallRPC(node, "getinfo");
  }
  catch(const runtime_error &e)
  {
	return;
  }
  newHeight = find_value(r.get_obj(), "blocks").get_int() + nBlocks;
  const string &sBlocks = strprintf("%d",nBlocks);
  BOOST_CHECK_NO_THROW(r = CallRPC(node, "generate " + sBlocks));
  BOOST_CHECK_NO_THROW(r = CallRPC(node, "getinfo"));
  height = find_value(r.get_obj(), "blocks").get_int();
  BOOST_CHECK(height >= newHeight);
  height = 0;
  timeoutCounter = 0;
  while(!otherNode1.empty() && height < newHeight)
  {
	  MilliSleep(100);
	  try
	  {
		r = CallRPC(otherNode1, "getinfo");
	  }
	  catch(const runtime_error &e)
	  {
		r = NullUniValue;
	  }
	  if(!r.isObject())
	  {
		 height = newHeight;
		 break;
	  }
	  height = find_value(r.get_obj(), "blocks").get_int();
	  timeoutCounter++;
	  if (timeoutCounter > 300) {
		  printf("Error: Timeout on getinfo for %s, height %d vs newHeight %d!\n", otherNode1.c_str(), height, newHeight);
		  break;
	  }
  }
  if(!otherNode1.empty())
	BOOST_CHECK(height >= newHeight);
  height = 0;
  timeoutCounter = 0;
  while(!otherNode2.empty() &&height < newHeight)
  {
	  MilliSleep(100);
	  try
	  {
		r = CallRPC(otherNode2, "getinfo");
	  }
	  catch(const runtime_error &e)
	  {
		r = NullUniValue;
	  }
	  if(!r.isObject())
	  {
		 height = newHeight;
		 break;
	  }
	  height = find_value(r.get_obj(), "blocks").get_int();
	  timeoutCounter++;
	  if (timeoutCounter > 300) {
		printf("Error: Timeout on getinfo for %s, height %d vs newHeight %d!\n", otherNode2.c_str(), height, newHeight);
		break;
	  }
  }
  if(!otherNode2.empty())
	BOOST_CHECK(height >= newHeight);
  height = 0;
  timeoutCounter = 0;
}
void SetSysMocktime(const int64_t& expiryTime) {
	BOOST_CHECK(expiryTime > 0);
	string cmd = strprintf("setmocktime %lld", expiryTime);
	try
	{
		CallRPC("node1", "getinfo");
		BOOST_CHECK_NO_THROW(CallRPC("node1", cmd, true, false));
		GenerateBlocks(5, "node1");
		GenerateBlocks(5, "node1");
		GenerateBlocks(5, "node1");
		UniValue r;
		BOOST_CHECK_NO_THROW(r = CallRPC("node1", "getblockchaininfo"));
		BOOST_CHECK(expiryTime <= find_value(r.get_obj(), "mediantime").get_int64());
	}
	catch (const runtime_error &e)
	{
	}
	try
	{
		CallRPC("node2", "getinfo");
		BOOST_CHECK_NO_THROW(CallRPC("node2", cmd, true, false));
		GenerateBlocks(5, "node2");
		GenerateBlocks(5, "node2");
		GenerateBlocks(5, "node2");
		UniValue r;
		BOOST_CHECK_NO_THROW(r = CallRPC("node2", "getblockchaininfo"));
		BOOST_CHECK(expiryTime <= find_value(r.get_obj(), "mediantime").get_int64());
	}
	catch (const runtime_error &e)
	{
	}
	try
	{
		CallRPC("node3", "getinfo");
		BOOST_CHECK_NO_THROW(CallRPC("node3", cmd, true, false));
		GenerateBlocks(5, "node3");
		GenerateBlocks(5, "node3");
		GenerateBlocks(5, "node3");
		UniValue r;
		BOOST_CHECK_NO_THROW(r = CallRPC("node3", "getblockchaininfo"));
		BOOST_CHECK(expiryTime <= find_value(r.get_obj(), "mediantime").get_int64());
	}
	catch (const runtime_error &e)
	{
	}
}
void ExpireAlias(const string& alias)
{
	int64_t expiryTime = 0;
	// ensure alias is expired
	UniValue r;
	try
	{
		UniValue aliasres;
		aliasres = CallRPC("node1", "aliasinfo " + alias);
		expiryTime = find_value(aliasres.get_obj(), "expires_on").get_int64();
		SetSysMocktime(expiryTime + 2);
		r = CallRPC("node1", "aliasinfo " + alias);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);
	}
	catch(const runtime_error &e)
	{
	}
	try
	{
		if (expiryTime <= 0) {
			UniValue aliasres;
			aliasres = CallRPC("node2", "aliasinfo " + alias);
			expiryTime = find_value(aliasres.get_obj(), "expires_on").get_int64();
			SetSysMocktime(expiryTime + 2);
		}
		r = CallRPC("node2", "aliasinfo " + alias);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);
	}
	catch(const runtime_error &e)
	{
	}
	try
	{
		if (expiryTime <= 0) {
			UniValue aliasres;
			aliasres = CallRPC("node3", "aliasinfo " + alias);
			expiryTime = find_value(aliasres.get_obj(), "expires_on").get_int64();
			SetSysMocktime(expiryTime + 2);
		}
		r = CallRPC("node3", "aliasinfo " + alias);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), true);
	}
	catch(const runtime_error &e)
	{
	}
}
void GetOtherNodes(const string& node, string& otherNode1, string& otherNode2)
{
	otherNode1 = "";
	otherNode2 = "";
	if(node == "node1")
	{
		if(node2Online)
			otherNode1 = "node2";
		if(node3Online)
			otherNode2 = "node3";
	}
	else if(node == "node2")
	{
		if(node1Online)
			otherNode1 = "node1";
		if(node3Online)
			otherNode2 = "node3";
	}
	else if(node == "node3")
	{
		if(node1Online)
			otherNode1 = "node1";
		if(node2Online)
			otherNode2 = "node2";
	}

}
string AliasNew(const string& node, const string& aliasname, const string& pubdata, string witness)
{
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);

	CKey privEncryptionKey;
	privEncryptionKey.MakeNewKey(true);
	CPubKey pubEncryptionKey = privEncryptionKey.GetPubKey();
	vector<unsigned char> vchPrivEncryptionKey(privEncryptionKey.begin(), privEncryptionKey.end());
	vector<unsigned char> vchPubEncryptionKey(pubEncryptionKey.begin(), pubEncryptionKey.end());

	vector<unsigned char> vchPubKey;
	CKey privKey;
	vector<unsigned char> vchPasswordSalt;

	privKey.MakeNewKey(true);
	CPubKey pubKey = privKey.GetPubKey();
	vchPubKey = vector<unsigned char>(pubKey.begin(), pubKey.end());
	CSyscoinAddress aliasAddress(pubKey.GetID());
	vector<unsigned char> vchPrivKey(privKey.begin(), privKey.end());
	BOOST_CHECK(privKey.IsValid());
	BOOST_CHECK(privEncryptionKey.IsValid());
	BOOST_CHECK(pubKey.IsFullyValid());
	BOOST_CHECK_NO_THROW(CallRPC(node, "importprivkey " + CSyscoinSecret(privKey).ToString() + " \"\" false", true, false));
	BOOST_CHECK_NO_THROW(CallRPC(node, "importprivkey " + CSyscoinSecret(privEncryptionKey).ToString() + " \"\" false", true, false));

	string strEncryptionPrivateKeyHex = HexStr(vchPrivEncryptionKey);
	string expires = "\"\"";
	string aliases = "\"\"";
	string acceptTransfers = "\"\"";
	string expireTime = "\"\"";

	UniValue r;
	// registration
	BOOST_CHECK_NO_THROW(CallRPC(node, "aliasnew " + aliasname + " " + pubdata + " " + acceptTransfers +  " " + expireTime + " " + aliasAddress.ToString() + " " + strEncryptionPrivateKeyHex + " " + HexStr(vchPubEncryptionKey) + " " + witness));
	GenerateBlocks(5, node);
	// activation
	BOOST_CHECK_NO_THROW(CallRPC(node, "aliasnew " + aliasname + " \"\""));
	GenerateBlocks(5, node);
	BOOST_CHECK_THROW(CallRPC(node, "sendtoaddress " + aliasname + " 10"), runtime_error);
	GenerateBlocks(5, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + aliasname));
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK(balanceAfter >= 10*COIN);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + aliasname));
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == aliasname);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), pubdata);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str() , aliasAddress.ToString());
	if(!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "aliasinfo " + aliasname));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == aliasname);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), pubdata);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str() , aliasAddress.ToString());
	}
	if(!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "aliasinfo " + aliasname));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == aliasname);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str(), pubdata);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str() , aliasAddress.ToString());
	}
	return aliasAddress.ToString();
}
string AliasTransfer(const string& node, const string& aliasname, const string& tonode, const string& pubdata, const string& witness)
{
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + aliasname));

	string oldvalue = find_value(r.get_obj(), "publicvalue").get_str();

	string encryptionkey = find_value(r.get_obj(), "encryption_publickey").get_str();
	string encryptionprivkey = find_value(r.get_obj(), "encryption_privatekey").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + aliasname));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));

	CKey privKey, encryptionPrivKey;
	privKey.MakeNewKey(true);
	CPubKey pubKey = privKey.GetPubKey();
	vector<unsigned char> vchPubKey(pubKey.begin(), pubKey.end());
	CSyscoinAddress aliasAddress(pubKey.GetID());
	vector<unsigned char> vchPrivKey(privKey.begin(), privKey.end());
	BOOST_CHECK(privKey.IsValid());
	BOOST_CHECK(pubKey.IsFullyValid());
	BOOST_CHECK_NO_THROW(CallRPC(tonode, "importprivkey " + CSyscoinSecret(privKey).ToString() + " \"\" false", true, false));	

	string acceptTransfers = "\"\"";
	string expires = "\"\"";
	string address = aliasAddress.ToString();
	string password = "\"\"";
	string encryptionpubkey = "\"\"";

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasupdate " + aliasname + " " + pubdata + " " + address + " " + acceptTransfers + " " + expires + " " + encryptionpubkey + " " + encryptionpubkey + " " + witness));
	const UniValue& resArray = r.get_array();
	if(resArray.size() > 1)
	{
		const UniValue& complete_value = resArray[1];
		bool bComplete = false;
		if (complete_value.isStr())
			bComplete = complete_value.get_str() == "true";
		if(!bComplete)
		{
			string hex_str = resArray[0].get_str();	
			return hex_str;
		}
	}	
	GenerateBlocks(5, tonode);
	GenerateBlocks(5, node);

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + aliasname));
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + aliasname));
	BOOST_CHECK(balanceAfter >= (balanceBefore-COIN));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str() , pubdata != "\"\""? pubdata: oldvalue);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "encryption_publickey").get_str() , encryptionkey);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "encryption_privatekey").get_str() , encryptionprivkey);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str() , aliasAddress.ToString());

	// check xferred right person and data changed
	BOOST_CHECK_NO_THROW(r = CallRPC(tonode, "aliasbalance " + aliasname));
	balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_NO_THROW(r = CallRPC(tonode, "aliasinfo " + aliasname));
	BOOST_CHECK(balanceAfter >= (balanceBefore-COIN));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str() , pubdata != "\"\""? pubdata: oldvalue);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "encryption_publickey").get_str() , encryptionkey);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "encryption_privatekey").get_str() , encryptionprivkey);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str() , aliasAddress.ToString());
	return "";
}
string AliasUpdate(const string& node, const string& aliasname, const string& pubdata, string addressStr, string witness)
{
	string addressStr1 = addressStr;
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + aliasname));

	string oldvalue = find_value(r.get_obj(), "publicvalue").get_str();
	string oldAddressStr = find_value(r.get_obj(), "address").get_str();
	string encryptionkey = find_value(r.get_obj(), "encryption_publickey").get_str();
	string encryptionprivkey = find_value(r.get_obj(), "encryption_privatekey").get_str();
	
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + aliasname));
	CAmount balanceBefore = AmountFromValue(find_value(r.get_obj(), "balance"));


	string acceptTransfers = "\"\"";
	string expires = "\"\"";
	string encryptionpubkey = "\"\"";
	// "aliasupdate <aliasname> [public value]  [address] [accept_transfers=true] [expire_timestamp] [encryption_privatekey] [encryption_publickey] [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasupdate " + aliasname + " " + pubdata + " " + addressStr + " " + acceptTransfers + " " + expires + " " + encryptionpubkey + " " + encryptionpubkey + " " + witness));
	const UniValue& resArray = r.get_array();
	if(resArray.size() > 1)
	{
		const UniValue& complete_value = resArray[1];
		bool bComplete = false;
		if (complete_value.isStr())
			bComplete = complete_value.get_str() == "true";
		if(!bComplete)
		{
			string hex_str = resArray[0].get_str();	
			return hex_str;
		}
	}
	GenerateBlocks(5, node);
	GenerateBlocks(5, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + aliasname));
	CAmount balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + aliasname));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str() , addressStr != "\"\""? addressStr: oldAddressStr);
	
	BOOST_CHECK(abs(balanceBefore-balanceAfter) < COIN);
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == aliasname);

	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str() , pubdata != "\"\""? pubdata: oldvalue);
	
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "encryption_publickey").get_str() , encryptionkey);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "encryption_privatekey").get_str() , encryptionprivkey);


	
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);
	if(!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "aliasbalance " + aliasname));
		balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "aliasinfo " + aliasname));
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str() , addressStr != "\"\""? addressStr: oldAddressStr);
		
		BOOST_CHECK(abs(balanceBefore-balanceAfter) < COIN);	
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == aliasname);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);

		
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str() , pubdata != "\"\""? pubdata: oldvalue);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "encryption_publickey").get_str() , encryptionkey);

	}
	if(!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "aliasbalance " + aliasname));
		balanceAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "aliasinfo " + aliasname));
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "address").get_str() , addressStr != "\"\""? addressStr: oldAddressStr);
		
		BOOST_CHECK(abs(balanceBefore-balanceAfter) < COIN);
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == aliasname);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "expired").get_bool(), false);

		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str() , pubdata != "\"\""? pubdata: oldvalue);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "encryption_publickey").get_str() , encryptionkey);
	}
	return "";

}
void AliasAddWhitelist(const string& node, const string& owneralias, const string& aliasname, const string& discount, const string& witness)
{
	bool found = false;
	UniValue r;
	string whiteListArray = "\"[{\\\"alias\\\":\\\"" + aliasname + "\\\",\\\"discount_percentage\\\":" + discount + "}]\"";
	BOOST_CHECK_NO_THROW(CallRPC(node, "aliasupdatewhitelist " + owneralias + " " + whiteListArray + " " + witness));
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliaswhitelist " + owneralias));
	const UniValue &arrayValue = r.get_array();
	for (int i = 0; i<arrayValue.size(); i++)
	{
		const string &aliasguid = find_value(arrayValue[i].get_obj(), "alias").get_str();
		if (aliasguid == aliasname)
		{
			found = true;
			BOOST_CHECK_EQUAL(find_value(arrayValue[i].get_obj(), "discount_percentage").get_int(), atoi(discount));
		}
	}
	BOOST_CHECK(found);
}
void AliasRemoveWhitelist(const string& node, const string& owneralias, const string& aliasname, const string& discount, const string& witness)
{
	UniValue r;
	string whiteListArray = "\"[{\\\"alias\\\":\\\"" + aliasname + "\\\",\\\"discount_percentage\\\":" + discount + "}]\"";
	BOOST_CHECK_NO_THROW(CallRPC(node, "aliasupdatewhitelist " + owneralias + " " + whiteListArray + " " + witness));
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliaswhitelist " + owneralias));
	const UniValue &arrayValue = r.get_array();
	for (int i = 0; i<arrayValue.size(); i++)
	{
		const string &aliasguid = find_value(arrayValue[i].get_obj(), "alias").get_str();
		BOOST_CHECK(aliasguid != aliasname);
	}
}
void AliasClearWhitelist(const string& node, const string& owneralias, const string &witness)
{
	UniValue r;
	BOOST_CHECK_NO_THROW(CallRPC(node, "aliasclearwhitelist " + owneralias + " " + witness));
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliaswhitelist " + owneralias));
	const UniValue &arrayValue = r.get_array();
	BOOST_CHECK(arrayValue.empty());

}
int FindAliasDiscount(const string& node, const string& owneralias, const string &aliasname) 
{
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliaswhitelist " + owneralias));
	const UniValue &arrayValue = r.get_array();
	for (int i = 0; i<arrayValue.size(); i++)
	{
		const string &aliasguid = find_value(arrayValue[i].get_obj(), "alias").get_str();
		if (aliasguid == aliasname)
		{
			return find_value(arrayValue[i].get_obj(), "discount_percentage").get_int();
		}
	}
	return 0;
}
bool AliasFilter(const string& node, const string& alias)
{
	UniValue r;
	int64_t currentTime = GetTime();
	string query = "\"{\\\"_id\\\":\\\"" + alias + "\\\"}\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "syscoinquery alias " + query));
	const UniValue &arr = r.get_array();
	return !arr.empty();
}
bool OfferFilter(const string& node, const string& offer)
{
	UniValue r;
	int64_t currentTime = GetTime();
	string query = "\"{\\\"_id\\\":\\\"" + offer + "\\\"}\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "syscoinquery offer " + query));
	const UniValue &arr = r.get_array();
	return !arr.empty();
}
bool CertFilter(const string& node, const string& cert)
{
	UniValue r;
	int64_t currentTime = GetTime();
	string query = "\"{\\\"_id\\\":\\\"" + cert + "\\\"}\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "syscoinquery cert " + query));
	const UniValue &arr = r.get_array();
	return !arr.empty();
}
bool EscrowFilter(const string& node, const string& escrow)
{
	UniValue r;
	string query = "\"{\\\"_id\\\":\\\"" + escrow + "\\\"}\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "syscoinquery escrow " + query));
	const UniValue &arr = r.get_array();
	return !arr.empty();
}
UniValue EscrowBidFilter(const string& node, const string& txid)
{
	UniValue r;
	string query = "\"{\\\"_id\\\":\\\"" + txid + "\\\"}\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "syscoinquery escrowbid " + query));
	const UniValue &arr = r.get_array();
	return arr;
}
UniValue EscrowBidFilterFromGUID(const string& node, const string& guid)
{
	UniValue r;
	string query = "\"{\\\"escrow\\\":\\\"" + guid + "\\\"}\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "syscoinquery escrowbid " + query));
	const UniValue &arr = r.get_array();
	return arr;
}
const string CertNew(const string& node, const string& alias, const string& title, const string& pubdata, const string& witness)
{
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + alias));


	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certnew " + alias + " " + title + " " + pubdata + " " + " certificates " + witness));
	const UniValue &arr = r.get_array();
	string guid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == alias);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "publicvalue").get_str() == pubdata);
	if(!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "certinfo " + guid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		BOOST_CHECK(find_value(r.get_obj(), "publicvalue").get_str() == pubdata);
		BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	}
	if(!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "certinfo " + guid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		BOOST_CHECK(find_value(r.get_obj(), "publicvalue").get_str() == pubdata);
		BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	}
	return guid;
}
void CertUpdate(const string& node, const string& guid, const string& title, const string& pubdata, const string& witness)
{
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	string oldalias = find_value(r.get_obj(), "alias").get_str();
	string oldpubdata = find_value(r.get_obj(), "publicvalue").get_str();
	string oldtitle = find_value(r.get_obj(), "title").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + oldalias));

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certupdate " + guid + " " + title + " " + pubdata + " certificates " + witness));
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == oldalias);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "title").get_str(), title != "\"\""? title: oldtitle);

	if(!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "certinfo " + guid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == oldalias);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str() , pubdata != "\"\""? pubdata: oldpubdata);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "title").get_str(), title != "\"\""? title: oldtitle);

	}
	if(!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "certinfo " + guid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == oldalias);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "publicvalue").get_str() , pubdata != "\"\""? pubdata: oldpubdata);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "title").get_str(), title != "\"\""? title: oldtitle);

	}
}
void CertTransfer(const string& node, const string &tonode, const string& guid, const string& toalias, const string& witness)
{
	UniValue r;

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	string pubdata = find_value(r.get_obj(), "publicvalue").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasinfo " + toalias));


	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certtransfer " + guid + " " + toalias + " " + pubdata + " " + witness));
	GenerateBlocks(5, node);
	GenerateBlocks(5, tonode);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "publicvalue").get_str() == pubdata);
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);

	BOOST_CHECK_NO_THROW(r = CallRPC(tonode, "certinfo " + guid));
	BOOST_CHECK(find_value(r.get_obj(), "alias").get_str() == toalias);
	BOOST_CHECK(find_value(r.get_obj(), "publicvalue").get_str() == pubdata);

}
const string OfferLink(const string& node, const string& alias, const string& guid, const string& commissionStr, const string& newdetails, const string &witness)
{
	UniValue r;
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + guid));
	const string &olddetails = find_value(r.get_obj(), "description").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerlink " + alias + " " + guid + " " + commissionStr + " " + newdetails + " " + witness));
	const UniValue &arr = r.get_array();
	string linkedguid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + linkedguid));
	if(!newdetails.empty())
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == newdetails);
	else
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == olddetails);
	int commission = atoi(commissionStr.c_str());
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == linkedguid);
	BOOST_CHECK(find_value(r.get_obj(), "offerlink_guid").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "commission").get_int() == commission);
	if(!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "offerinfo " + linkedguid));
		if(!newdetails.empty())
			BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == newdetails);
		else
			BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == olddetails);
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == linkedguid);
	}
	if(!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "offerinfo " + linkedguid));
		if(!newdetails.empty())
			BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == newdetails);
		else
			BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == olddetails);
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == linkedguid);
	}
	return linkedguid;
}
const string OfferNew(const string& node, const string& aliasname, const string& category, const string& title, const string& qtyStr, const string& price, const string& description, const string& currency, const string& certguid, const string& paymentoptions, const string& offerType, const string& auction_expires, const string& auction_reserve, const string& auction_require_witness, const string &auction_deposit, const string& witness)
{
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	string pvt = "\"\"";
	string units = "\"\"";
	int qty = atoi(qtyStr.c_str());
	UniValue r;
	//						"offernew <alias> <category> <title> <quantity> <price> <description> <currency> [cert. guid] [payment options=SYS] [private=false] [units] [offerType=BUYNOW] [auction_expires] [auction_reserve] [auction_require_witness] [auction_deposit] [witness]\n"
	string offercreatestr = "offernew " + aliasname + " " + category + " " + title + " " + qtyStr + " " + price + " " + description + " " + currency  + " " + certguid + " " + paymentoptions + " " + pvt + " " + units + " " + offerType + " " + auction_expires + " " + auction_reserve + " " + auction_require_witness + " " + auction_deposit + " " + witness;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, offercreatestr));
	const UniValue &arr = r.get_array();
	string guid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + guid));

	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
	if(certguid != "\"\"")
		BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == certguid);

	bool auctionreqwitness = (auction_require_witness == "true") ? true : false;
	BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
	BOOST_CHECK(find_value(r.get_obj(), "currency").get_str() == currency);
	float compareprice = 0;
	if (price != "\"\"")
		compareprice = boost::lexical_cast<float>(price);

	BOOST_CHECK(abs(find_value(r.get_obj(), "price").get_real() - compareprice) < 0.001);
	BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
	BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == description);
	if(offerType != "\"\"")
		BOOST_CHECK(find_value(r.get_obj(), "offertype").get_str() == offerType);

	if (auction_expires != "\"\"")
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_expires_on").get_int64(), boost::lexical_cast<long>(auction_expires));
	if (auction_reserve != "\"\"")
		BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_reserve_price").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_reserve) * 1000)));
	if (auction_require_witness != "\"\"")
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_require_witness").get_bool(), auctionreqwitness);
	if (auction_deposit != "\"\"")
		BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_deposit").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_deposit) * 1000)));
	
	if(!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "offerinfo " + guid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		if(certguid != "\"\"")
			BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == certguid);

		BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
		BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == description);
		BOOST_CHECK(find_value(r.get_obj(), "currency").get_str() == currency);
		float compareprice = 0;
		if (price != "\"\"")
			compareprice = boost::lexical_cast<float>(price);

		BOOST_CHECK(abs(find_value(r.get_obj(), "price").get_real() - compareprice) < 0.001);
		if (offerType != "\"\"")
			BOOST_CHECK(find_value(r.get_obj(), "offertype").get_str() == offerType);

		if (auction_expires != "\"\"")
			BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_expires_on").get_int64(), boost::lexical_cast<long>(auction_expires));
		if (auction_reserve != "\"\"")
			BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_reserve_price").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_reserve) * 1000)));
		if (auction_require_witness != "\"\"")
			BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_require_witness").get_bool(), auctionreqwitness);
		if (auction_deposit != "\"\"")
			BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_deposit").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_deposit) * 1000)));
	}
	if(!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "offerinfo " + guid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		if(certguid != "\"\"")
			BOOST_CHECK(find_value(r.get_obj(), "cert").get_str() == certguid);
		BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
		BOOST_CHECK(find_value(r.get_obj(), "title").get_str() == title);
		BOOST_CHECK(find_value(r.get_obj(), "description").get_str() == description);
		BOOST_CHECK(find_value(r.get_obj(), "currency").get_str() == currency);
		float compareprice = 0;
		if (price != "\"\"")
			compareprice = boost::lexical_cast<float>(price);
	
		BOOST_CHECK(abs(find_value(r.get_obj(), "price").get_real() - compareprice) < 0.001);

		if (offerType != "\"\"")
			BOOST_CHECK(find_value(r.get_obj(), "offertype").get_str() == offerType);

		if (auction_expires != "\"\"")
			BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_expires_on").get_int64(), boost::lexical_cast<long>(auction_expires));
		if (auction_reserve != "\"\"")
			BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_reserve_price").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_reserve) * 1000)));
		if (auction_require_witness != "\"\"")
			BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_require_witness").get_bool(), auctionreqwitness);
		if (auction_deposit != "\"\"")
			BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_deposit").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_deposit) * 1000)));
	}
	return guid;
}

void OfferUpdate(const string& node, const string& aliasname, const string& offerguid, const string& category, const string& title, const string& qtyStr, const string& price, const string& description, const string& currency, const string &isprivateStr, const string& certguid, const string& commissionStr, const string& paymentoptions, const string& offerType, const string& auction_expires, const string& auction_reserve, const string& auction_require_witness, const string &auction_deposit, const string& witness) {
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offerguid));
	int oldqty = find_value(r.get_obj(), "quantity").get_int();
	int qty = atoi(qtyStr.c_str());
	float oldprice = find_value(r.get_obj(), "price").get_real();
	string oldcurrency = find_value(r.get_obj(), "currency").get_str();
	bool oldprivate = find_value(r.get_obj(), "private").get_bool();
	bool isprivate = isprivateStr == "true";
	string oldcert = find_value(r.get_obj(), "cert").get_str();
	int oldcommission = find_value(r.get_obj(), "commission").get_int();
	int commission = atoi(commissionStr.c_str());
	string oldpaymentoptions = find_value(r.get_obj(), "paymentoptions").get_str();
	string olddescription = find_value(r.get_obj(), "description").get_str();
	string oldtitle = find_value(r.get_obj(), "title").get_str();
	string oldcategory = find_value(r.get_obj(), "category").get_str();
	//						"offerupdate <alias> <guid> [category] [title] [quantity] [price] [description] [currency] [private=false] [cert. guid] [commission] [paymentOptions] [offerType=BUYNOW] [auction_expires] [auction_reserve] [auction_require_witness] [auction_deposit] [witness]\n"
	string offerupdatestr = "offerupdate " + aliasname + " " + offerguid + " " + category + " " + title + " " + qtyStr + " " + price + " " + description + " " + currency + " " + isprivateStr + " " + certguid + " " +  commissionStr + " " + paymentoptions + " " + offerType + " " + auction_expires + " " + auction_reserve + " " + auction_require_witness + " " + auction_deposit + " " + witness;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, offerupdatestr));
	GenerateBlocks(10, node);
	bool auctionreqwitness = (auction_require_witness == "true") ? true : false;

		
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offerguid));
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "_id").get_str() , offerguid);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "cert").get_str() , certguid != "\"\""? certguid: oldcert);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "quantity").get_int() , qtyStr != "\"\""? qty: oldqty);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "currency").get_str() , currency != "\"\""? currency: oldcurrency);
	float compareprice = 0;
	if (price != "\"\"")
		compareprice = boost::lexical_cast<float>(price);
	else
		compareprice = oldprice;
	BOOST_CHECK(abs(find_value(r.get_obj(), "price").get_real() - compareprice) < 0.001);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "commission").get_int() , commissionStr != "\"\""? commission: oldcommission);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "paymentoptions").get_str() , paymentoptions != "\"\""? paymentoptions: oldpaymentoptions);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "private").get_bool() , isprivateStr != "\"\""? isprivate: oldprivate);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "description").get_str(), description != "\"\""? description: olddescription);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "title").get_str(), title != "\"\""? title: oldtitle);
	BOOST_CHECK_EQUAL(find_value(r.get_obj(), "category").get_str(), category != "\"\""? category: oldcategory);
	if (offerType != "\"\"")
		BOOST_CHECK(find_value(r.get_obj(), "offertype").get_str() == offerType);

	if (auction_expires != "\"\"")
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_expires_on").get_int64(), boost::lexical_cast<long>(auction_expires));
	if (auction_reserve != "\"\"")
		BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_reserve_price").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_reserve) * 1000)));
	if (auction_require_witness != "\"\"")
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_require_witness").get_bool(), auctionreqwitness);
	if (auction_deposit != "\"\"")
		BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_deposit").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_deposit) * 1000)));
	if(!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "offerinfo " + offerguid));
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "_id").get_str() , offerguid);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "cert").get_str() , certguid != "\"\""? certguid: oldcert);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "quantity").get_int() , qtyStr != "\"\""? qty: oldqty);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "currency").get_str() , currency != "\"\""? currency: oldcurrency);
		float compareprice = 0;
		if (price != "\"\"")
			compareprice = boost::lexical_cast<float>(price);
		else
			compareprice = oldprice;
		BOOST_CHECK(abs(find_value(r.get_obj(), "price").get_real() - compareprice) < 0.001);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "commission").get_int() , commissionStr != "\"\""? commission: oldcommission);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "paymentoptions").get_str() , paymentoptions != "\"\""? paymentoptions: oldpaymentoptions);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "private").get_bool() , isprivateStr != "\"\""? isprivate: oldprivate);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "description").get_str(), description != "\"\""? description: olddescription);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "title").get_str(), title != "\"\""? title: oldtitle);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "category").get_str(), category != "\"\""? category: oldcategory);
		if (offerType != "\"\"")
			BOOST_CHECK(find_value(r.get_obj(), "offertype").get_str() == offerType);

		if (auction_expires != "\"\"")
			BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_expires_on").get_int64(), boost::lexical_cast<long>(auction_expires));
		if (auction_reserve != "\"\"")
			BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_reserve_price").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_reserve) * 1000)));
		if (auction_require_witness != "\"\"")
			BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_require_witness").get_bool(), auctionreqwitness);
		if (auction_deposit != "\"\"")
			BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_deposit").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_deposit) * 1000)));
	}
	if(!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "offerinfo " + offerguid));
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "_id").get_str() , offerguid);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "cert").get_str() , certguid != "\"\""? certguid: oldcert);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "quantity").get_int() , qtyStr != "\"\""? qty: oldqty);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "currency").get_str() , currency != "\"\""? currency: oldcurrency);
		float compareprice = 0;
		if (price != "\"\"")
			compareprice = boost::lexical_cast<float>(price);
		else
			compareprice = oldprice;
			BOOST_CHECK(abs(find_value(r.get_obj(), "price").get_real() - compareprice) < 0.001);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "commission").get_int() , commissionStr != "\"\""? commission: oldcommission);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "paymentoptions").get_str() , paymentoptions != "\"\""? paymentoptions: oldpaymentoptions);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "private").get_bool() , isprivateStr != "\"\""? isprivate: oldprivate);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "description").get_str(), description != "\"\""? description: olddescription);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "title").get_str(), title != "\"\""? title: oldtitle);
		BOOST_CHECK_EQUAL(find_value(r.get_obj(), "category").get_str(), category != "\"\""? category: oldcategory);
		if (offerType != "\"\"")
			BOOST_CHECK(find_value(r.get_obj(), "offertype").get_str() == offerType);
		if (auction_expires != "\"\"")
			BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_expires_on").get_int64(), boost::lexical_cast<long>(auction_expires));
		if (auction_reserve != "\"\"")
			BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_reserve_price").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_reserve) * 1000)));
		if (auction_require_witness != "\"\"")
			BOOST_CHECK_EQUAL(find_value(r.get_obj(), "auction_require_witness").get_bool(), auctionreqwitness);
		if (auction_deposit != "\"\"")
			BOOST_CHECK_EQUAL(((int)(find_value(r.get_obj(), "auction_deposit").get_real() * 1000 + 0.5)), ((int)(boost::lexical_cast<float>(auction_deposit) * 1000)));
	}
}

void EscrowFeedback(const string& node, const string& role, const string& escrowguid, const string& feedback, const string& rating,  char user, const string& witness) {

	UniValue r, ret;
	string escrowfeedbackstr = "escrowfeedback " + escrowguid + " " + role + " " + feedback + " " + rating  + witness;

	BOOST_CHECK_NO_THROW(r = CallRPC(node, escrowfeedbackstr));
	const UniValue &arr = r.get_array();
	string escrowTxid = arr[0].get_str();

	GenerateBlocks(10, node);
	
	r = FindFeedback(node, escrowTxid);
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == escrowguid + user);
	BOOST_CHECK(find_value(r.get_obj(), "escrow").get_str() == escrowguid);
	BOOST_CHECK(find_value(r.get_obj(), "txid").get_str() == escrowTxid);
	BOOST_CHECK(find_value(r.get_obj(), "rating").get_int() == atoi(rating.c_str()));
	BOOST_CHECK(find_value(r.get_obj(), "feedback").get_str() == rating);
	BOOST_CHECK(find_value(r.get_obj(), "feedbackto").get_int() == user);
}
const string OfferAccept(const string& ownernode, const string& buyernode, const string& aliasname, const string& arbiter, const string& offerguid, const string& qty, const string& witness) {
	const string &escrowguid = EscrowNewBuyItNow(buyernode, ownernode, aliasname, offerguid, qty, arbiter);
	EscrowRelease(buyernode, "buyer", escrowguid);
	EscrowClaimRelease(ownernode, escrowguid);
	return escrowguid;
}
void EscrowBid(const string& node, const string& buyeralias, const string& escrowguid, const string& bid_in_offer_currency, const string &witness)
{
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	UniValue r, ret;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + escrowguid));
	string offerguid = find_value(r.get_obj(), "offer").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offerguid));
	string currency = find_value(r.get_obj(), "currency").get_str();

	float fPaymentCurrency = boost::lexical_cast<float>(bid_in_offer_currency);
	const string &bid_in_offer_currency1 = strprintf("%.*f", 8, fPaymentCurrency);
	const string &bid_in_payment_option1 = strprintf("%.*f", 8, strprintf("%.*f", 8, pegRates[currency] * fPaymentCurrency));
	//										"escrowbid <alias> <escrow> <bid_in_payment_option> <bid_in_offer_currency> [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowbid " + buyeralias + " " + escrowguid + " " + bid_in_payment_option1 + " " + bid_in_offer_currency1 + " " + witness));
	const UniValue &arr = r.get_array();
	string txid = arr[0].get_str();
	GenerateBlocks(10, node);
	const UniValue &escrowBid = EscrowBidFilter(node, txid);
	BOOST_CHECK(!escrowBid.empty());
	BOOST_CHECK(ret.read(escrowBid[0].get_str()));
	const UniValue &escrowBidObj = ret.get_obj();
	BOOST_CHECK(find_value(escrowBidObj, "_id").get_str() == txid);
	BOOST_CHECK_EQUAL(find_value(escrowBidObj, "bidder").get_str() , buyeralias);
	BOOST_CHECK(find_value(escrowBidObj, "escrow").get_str() == escrowguid);
	CAmount bidPaymentOption = AmountFromValue(bid_in_payment_option1);
	CAmount bidPaymentOptionObj = AmountFromValue(strprintf("%.*f", 8, boost::lexical_cast<float>(find_value(escrowBidObj, "bid_in_payment_option_per_unit").write())));
	BOOST_CHECK_EQUAL(bidPaymentOptionObj, bidPaymentOption);
	BOOST_CHECK_EQUAL(find_value(escrowBidObj, "witness").get_str() , witness != "\"\"" ? witness : "");
	BOOST_CHECK(find_value(escrowBidObj, "status").get_str() == "valid");


	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + escrowguid));
	int qty = find_value(r.get_obj(), "quantity").get_int();

	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == escrowguid);
	BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == false);
	BOOST_CHECK(AmountFromValue(find_value(r.get_obj(), "total_without_fee")) == bidPaymentOption*qty);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "total_or_bid_in_payment_option_per_unit")), bidPaymentOption);
	BOOST_CHECK(find_value(r.get_obj(), "buyer").get_str() == buyeralias);
	if (!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "escrowinfo " + escrowguid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == escrowguid);
		BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == false);
		BOOST_CHECK(AmountFromValue(find_value(r.get_obj(), "total_without_fee")) == bidPaymentOption*qty);
		BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "total_or_bid_in_payment_option_per_unit")), bidPaymentOption);
		BOOST_CHECK(find_value(r.get_obj(), "buyer").get_str() == buyeralias);
	}
	if (!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "escrowinfo " + escrowguid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == escrowguid);
		BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == false);
		BOOST_CHECK(AmountFromValue(find_value(r.get_obj(), "total_without_fee")) == bidPaymentOption*qty);
		BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "total_or_bid_in_payment_option_per_unit")), bidPaymentOption);
		BOOST_CHECK(find_value(r.get_obj(), "buyer").get_str() == buyeralias);
	}
}
const string EscrowNewAuction(const string& node, const string& sellernode, const string& buyeralias, const string& offerguid, const string& qtyStr, const string& bid_in_offer_currency, const string& arbiteralias, const string& shipping, const string& networkFee, const string& arbiterFee, const string& witnessFee, const string &witness)
{
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	UniValue r;
	int qty = atoi(qtyStr.c_str());
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + buyeralias));
	CAmount balanceBuyerBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offerguid));
	int nQtyBefore = find_value(r.get_obj(), "quantity").get_int();
	string selleralias = find_value(r.get_obj(), "alias").get_str();
	int icommission = find_value(r.get_obj(), "commission").get_int();

	string currency = find_value(r.get_obj(), "currency").get_str();
	BOOST_CHECK(pegRates.count(currency) > 0 && pegRates[currency] > 0);
	float fOfferPrice = find_value(r.get_obj(), "price").get_real();
	CAmount offerprice = AmountFromValue(strprintf("%.*f", 8, fOfferPrice * pegRates[currency]));
	CAmount nTotalOfferPrice = offerprice*qty;
	CAmount nEscrowFee = GetEscrowArbiterFee(nTotalOfferPrice, boost::lexical_cast<float>(arbiterFee == "\"\""? "0.005": arbiterFee));
	CAmount nWitnessFee = GetEscrowWitnessFee(nTotalOfferPrice, boost::lexical_cast<float>(witnessFee == "\"\"" ? "0" : witnessFee));
	CAmount nNetworkFee = getFeePerByte(PAYMENTOPTION_SYS) * 400;
	if (networkFee != "\"\"")
		nNetworkFee = boost::lexical_cast<int>(networkFee) * 400 * COIN;
	CAmount nShipping = AmountFromValue(shipping == "\"\"" ? "0" : shipping);
	string sellerlink_alias = find_value(r.get_obj(), "offerlink_seller").get_str();
	int discount = 0;
	// this step must be done in the UI,
	// check to ensure commission is correct
	if (!sellerlink_alias.empty())
	{
		selleralias = sellerlink_alias;
		discount = FindAliasDiscount("node1", selleralias, buyeralias);
	}
	CAmount nCommissionCompare = 0;
	int markup = discount + icommission;
	if (markup > 0)
		nCommissionCompare = nTotalOfferPrice*(markup/100);
	
	string exttxid = "\"\"";
	string paymentoptions = "\"\"";
	string redeemscript = "\"\"";
	string buyNowStr = "false";
	string strTotalInPaymentOption = ValueFromAmount(offerprice).write();

	float fPaymentCurrency = boost::lexical_cast<float>(bid_in_offer_currency);
	const string &bid_in_offer_currency1 = strprintf("%.*f", 8, fPaymentCurrency);
	const string &bid_in_payment_option1 = strprintf("%.*f", 8, strprintf("%.*f", 8, pegRates[currency] * fPaymentCurrency));


	//										"escrownew <getamountandaddress> <alias> <arbiter alias> <offer> <quantity> <buynow> <total_in_payment_option> [shipping amount] [network fee] [arbiter fee] [witness fee] [extTx] [payment option] [bid_in_payment_option] [bid_in_offer_currency] [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrownew false " + buyeralias + " " + arbiteralias + " " + offerguid + " " + qtyStr + " " + buyNowStr + " " + strTotalInPaymentOption + " " + shipping + " " + networkFee + " " + arbiterFee + " " + witnessFee + " " + exttxid + " " + paymentoptions + " " + bid_in_payment_option1 + " " + bid_in_offer_currency1 + " " + witness));
	const UniValue &arr = r.get_array();
	string guid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	CAmount nCommission = AmountFromValue(find_value(r.get_obj(), "commission"));
	// this step must be done in the UI, to ensure that the 'total_without_fee' parameter is the right price according to the bid_in_offer_currency_per_unit value converted into the offer currency
	// since the core doesn't know the rate conversions this must be done externally, the seller/buyer/arbiter should check prior to signing escrow transactions.
	CAmount nodeTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	nodeTotal = nodeTotal / pegRates[currency];
	BOOST_CHECK(abs(find_value(r.get_obj(), "bid_in_offer_currency_per_unit").get_real()*qty*COIN) - nodeTotal <= 0.1*COIN);

	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
	BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
	CAmount bidPaymentOption = AmountFromValue(bid_in_payment_option1);
	BOOST_CHECK_EQUAL(((int)find_value(r.get_obj(), "bid_in_offer_currency_per_unit").get_real()*100) , ((int)atof(bid_in_offer_currency1.c_str())*100));
	CAmount bidPaymentOptionObj = AmountFromValue(strprintf("%.*f", 8, boost::lexical_cast<float>(find_value(r.get_obj(), "total_or_bid_in_payment_option_per_unit").write())));
	BOOST_CHECK_EQUAL(bidPaymentOptionObj, bidPaymentOption);
	BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == false);
	BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
	BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
	BOOST_CHECK_EQUAL(nCommission , nCommissionCompare);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "arbiterfee")) , nEscrowFee);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "networkfee")) , nNetworkFee);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "witnessfee")) , nWitnessFee);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "shipping")) , nShipping);

	if (!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "escrowinfo " + guid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
		BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
		BOOST_CHECK_EQUAL(((int)find_value(r.get_obj(), "bid_in_offer_currency_per_unit").get_real() * 100), ((int)atof(bid_in_offer_currency1.c_str()) * 100));
		bidPaymentOptionObj = AmountFromValue(strprintf("%.*f", 8, boost::lexical_cast<float>(find_value(r.get_obj(), "total_or_bid_in_payment_option_per_unit").write())));
		BOOST_CHECK_EQUAL(bidPaymentOptionObj, bidPaymentOption);
		BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == false);
		BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
		BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
	}
	if (!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "escrowinfo " + guid));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
		BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
		BOOST_CHECK_EQUAL(((int)find_value(r.get_obj(), "bid_in_offer_currency_per_unit").get_real() * 100), ((int)atof(bid_in_offer_currency1.c_str()) * 100));
		bidPaymentOptionObj = AmountFromValue(strprintf("%.*f", 8, boost::lexical_cast<float>(find_value(r.get_obj(), "total_or_bid_in_payment_option_per_unit").write())));
		BOOST_CHECK_EQUAL(bidPaymentOptionObj, bidPaymentOption);
		BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == false);
		BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
		BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
	}

	BOOST_CHECK_NO_THROW(r = CallRPC(sellernode, "offerinfo " + offerguid));
	int nQtyAfter = find_value(r.get_obj(), "quantity").get_int();
	BOOST_CHECK_EQUAL(nQtyAfter, nQtyBefore);
	return guid;
}
const string EscrowNewBuyItNow(const string& node, const string& sellernode, const string& buyeralias, const string& offerguid, const string& qtyStr, const string& arbiteralias, const string& shipping, const string& networkFee, const string& arbiterFee, const string& witnessFee, const string &witness)
{
	string otherNode1, otherNode2;
	GetOtherNodes(node, otherNode1, otherNode2);
	UniValue r;
	int qty = atoi(qtyStr.c_str());
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + buyeralias));
	CAmount balanceBuyerBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offerguid));
	int nQtyBefore = find_value(r.get_obj(), "quantity").get_int();
	string selleralias = find_value(r.get_obj(), "alias").get_str();
	int icommission = find_value(r.get_obj(), "commission").get_int();
	string currency = find_value(r.get_obj(), "currency").get_str();
	BOOST_CHECK(pegRates.count(currency) > 0 && pegRates[currency] > 0);
	CAmount offerprice = AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "price").get_real() * pegRates[currency]));
	CAmount nTotalOfferPrice = offerprice*qty;
	CAmount nEscrowFee = GetEscrowArbiterFee(nTotalOfferPrice, boost::lexical_cast<float>(arbiterFee == "\"\"" ? "0.005" : arbiterFee));
	CAmount nWitnessFee = GetEscrowWitnessFee(nTotalOfferPrice, boost::lexical_cast<float>(witnessFee == "\"\"" ? "0.0" : witnessFee));
	CAmount nNetworkFee = getFeePerByte(PAYMENTOPTION_SYS)*400;
	if (networkFee != "\"\"")
		nNetworkFee = boost::lexical_cast<int>(networkFee)*400*COIN;
	CAmount nShipping = AmountFromValue(shipping == "\"\""? "0": shipping);
	string sellerlink_alias = find_value(r.get_obj(), "offerlink_seller").get_str();
	int discount = 0;
	// this step must be done in the UI,
	// check to ensure commission is correct
	if (!sellerlink_alias.empty())
	{
		selleralias = sellerlink_alias;
		discount = FindAliasDiscount("node1", selleralias, buyeralias);
	}
	CAmount nCommissionCompare = 0;
	int markup = discount + icommission;
	if (markup > 0)
		nCommissionCompare = nTotalOfferPrice*(markup / 100);

	string exttxid = "\"\"";
	string paymentoptions = "\"\"";
	string buyNowStr = "true";
	string strBidInOfferCurrency = "\"\"";
	string strBidInPaymentOption = "\"\"";
	string strDeposit = "\"\"";
	string strTotal = ValueFromAmount(offerprice).write();
	string strTotalInPaymentOption = ValueFromAmount(offerprice).write();
	//										"escrownew <getamountandaddress> <alias> <arbiter alias> <offer> <quantity> <buynow> <total_in_payment_option> [shipping amount] [network fee] [arbiter fee] [witness fee] [extTx] [payment option] [bid_in_payment_option] [bid_in_offer_currency] [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrownew false " + buyeralias + " " + arbiteralias + " " + offerguid + " " + qtyStr + " " + buyNowStr + " " + strTotalInPaymentOption + " " + shipping + " " + networkFee + " " + arbiterFee + " " + witnessFee + " " + exttxid + " " + paymentoptions + " " + strBidInPaymentOption + " " + strBidInPaymentOption + " " + witness));
	const UniValue &arr = r.get_array();
	const string &guid = arr[1].get_str();
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offerguid));
	int nQtyAfter = find_value(r.get_obj(), "quantity").get_int();
	BOOST_CHECK_EQUAL(nQtyAfter, nQtyBefore);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	CAmount nCommission = AmountFromValue(find_value(r.get_obj(), "commission"));
	// this step must be done in the UI, to ensure that the 'total_without_fee' parameter is the right price according to the offer_price value converted into the offer currency
	// since the core doesn't know the rate conversions this must be done externally, the seller/buyer/arbiter should check prior to signing escrow transactions.
	CAmount nodeTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	nodeTotal = nodeTotal / pegRates[currency];
	BOOST_CHECK(abs(AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "offer_price").get_real()*qty)) - nodeTotal) <= 0.1*COIN);
	BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
	BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
	BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
	BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == true);
	BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
	BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "total_without_fee")) , offerprice*qty);

	BOOST_CHECK_EQUAL(nCommission , nCommissionCompare);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "arbiterfee")) , nEscrowFee);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "networkfee")) , nNetworkFee);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "witnessfee")) , nWitnessFee);
	BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "shipping")) , nShipping);
	if(!otherNode1.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode1, "escrowinfo " + guid));
		nodeTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
		BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
		BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == true);
		BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
		BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
		BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "total_without_fee")) , offerprice*qty);

	}
	if(!otherNode2.empty())
	{
		BOOST_CHECK_NO_THROW(r = CallRPC(otherNode2, "escrowinfo " + guid));
		nodeTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
		BOOST_CHECK(find_value(r.get_obj(), "_id").get_str() == guid);
		BOOST_CHECK(find_value(r.get_obj(), "offer").get_str() == offerguid);
		BOOST_CHECK(find_value(r.get_obj(), "quantity").get_int() == qty);
		BOOST_CHECK(find_value(r.get_obj(), "buynow").get_bool() == true);
		BOOST_CHECK(find_value(r.get_obj(), "arbiter").get_str() == arbiteralias);
		BOOST_CHECK(find_value(r.get_obj(), "seller").get_str() == selleralias);
		BOOST_CHECK_EQUAL(AmountFromValue(find_value(r.get_obj(), "total_without_fee")) , offerprice*qty);
	}
	

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowacknowledge " + guid));
	const UniValue& resArray = r.get_array();
	if(resArray.size() > 1)
	{
		const UniValue& complete_value = resArray[1];
		bool bComplete = false;
		if (complete_value.isStr())
			bComplete = complete_value.get_str() == "true";
		BOOST_CHECK(!bComplete);
	}
	BOOST_CHECK_NO_THROW(r = CallRPC(sellernode, "escrowacknowledge " + guid));
	GenerateBlocks(10, sellernode);
	BOOST_CHECK_THROW(r = CallRPC(sellernode, "escrowacknowledge " + guid), runtime_error);
	BOOST_CHECK_NO_THROW(r = CallRPC(sellernode, "offerinfo " + offerguid));
	nQtyAfter = find_value(r.get_obj(), "quantity").get_int();
	BOOST_CHECK_EQUAL(nQtyAfter, nQtyBefore-qty);
	return guid;
}
void EscrowRelease(const string& node, const string& role, const string& guid ,const string& witness)
{
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	string offer = find_value(r.get_obj(), "offer").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offer));
	string currency = find_value(r.get_obj(), "currency").get_str();
	int nQtyOfferBefore = find_value(r.get_obj(), "quantity").get_int();
	float fOfferPrice = find_value(r.get_obj(), "price").get_real();
	string sellerlink_alias = find_value(r.get_obj(), "offerlink_seller").get_str();
	int icommission = find_value(r.get_obj(), "commission").get_int();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	int nQty = find_value(r.get_obj(), "quantity").get_int();
	string buyeralias = find_value(r.get_obj(), "buyer").get_str();
	float fPrice = find_value(r.get_obj(), "offer_price").get_real()*nQty;
	string redeemScriptStr = find_value(r.get_obj(), "redeem_script").get_str();
	CAmount nCommission = AmountFromValue(find_value(r.get_obj(), "commission"));
	// this step must be done in the UI, to ensure that the 'total_without_fee' parameter in escrownew is the right price according to the offer_price value converted into the offer currency
	// since the core doesn't know the rate conversions this must be done externally, the seller/buyer/arbiter should check prior to signing escrow transactions.
	CAmount nodeTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	nodeTotal = nodeTotal / pegRates[currency];
	BOOST_CHECK(abs(AmountFromValue(strprintf("%.*f", 8, fPrice)) - nodeTotal) <= 0.1*COIN);
	
	BOOST_CHECK(pegRates.count(currency) > 0 && pegRates[currency] > 0);
	CAmount offerprice = AmountFromValue(strprintf("%.*f", 8, fOfferPrice * pegRates[currency]));
	CAmount nTotalOfferPrice = offerprice*nQty;
	
	int discount = 0;
	string selleralias;
	// this step must be done in the UI,
	// check to ensure commission is correct
	if (!sellerlink_alias.empty())
	{
		selleralias = sellerlink_alias;
		discount = FindAliasDiscount("node1", selleralias, buyeralias);
	}
	CAmount nCommissionCompare = 0;
	int markup = discount + icommission;
	if (markup > 0)
		nCommissionCompare = nTotalOfferPrice*(markup / 100);

	bool bBuyNow = find_value(r.get_obj(), "buynow").get_bool();
	if (!bBuyNow) {
		const UniValue &escrowBidBefore = EscrowBidFilterFromGUID(node, guid);
		BOOST_CHECK(!escrowBidBefore.empty());
	}
	BOOST_CHECK_EQUAL(nCommission , nCommissionCompare);
	
	string escrowaddress = find_value(r.get_obj(), "escrowaddress").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "getaddressutxos \"{\\\"addresses\\\": [\\\"" + escrowaddress + "\\\"]}\""));
	UniValue addressUTXOsArray = r.get_array();
	// "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0,\\\"satoshis\\\":10000}]\"
	string inputStr = "\"[";
	for (unsigned int i = 0; i < addressUTXOsArray.size(); i++)
	{
		if (i > 0)
			inputStr += ",";
		const UniValue& utxoObj = addressUTXOsArray[i].get_obj();
		const string& txidStr = find_value(utxoObj.get_obj(), "txid").get_str();
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		CAmount satoshis = find_value(utxoObj.get_obj(), "satoshis").get_int64();
		inputStr += "{\\\"txid\\\":\\\"" + txidStr + "\\\",\\\"vout\\\":" + boost::lexical_cast<string>(nOut) + ",\\\"satoshis\\\":" + boost::lexical_cast<string>(satoshis) + "}";
	}
	inputStr += "]\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasaddscript " + redeemScriptStr));
	// create raw transaction, sign it and pass it to escrowrelease partially signed, escrow release will store the signatures and upon escrow claim, the buyer will sign and complete sending to network
	// "escrowcreaterawtransaction <type> <escrow guid> <user role> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]>\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowcreaterawtransaction release " + guid + " " + inputStr + " " + role));
	const UniValue &arr = r.get_array();
	string rawtx = arr[0].get_str();

	// UI should ensure value is >= 0  or else tell user it does not have enough funds in escrow address
	BOOST_CHECK(AmountFromValue(arr[1]) >= 0);

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "signrawtransaction " + rawtx));
	const UniValue& hex_value = find_value(r.get_obj(), "hex");
	BOOST_CHECK(hex_value.get_str() != rawtx);
	// "escrowrelease <escrow guid> <user role> <rawtx> [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowrelease " + guid + " " + role + " " + hex_value.get_str() + " " + witness));
	BOOST_CHECK(r.get_array().size() == 1);
	GenerateBlocks(10, node);
	GenerateBlocks(10, node);

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offer));
	int nQtyOfferAfter = find_value(r.get_obj(), "quantity").get_int();
	BOOST_CHECK_EQUAL(nQtyOfferAfter, nQtyOfferBefore);
	if (!bBuyNow) {
		const UniValue &escrowBidAfter = EscrowBidFilterFromGUID(node, guid);
		BOOST_CHECK(!escrowBidAfter.empty());
	}
}
void EscrowRefund(const string& node, const string& role, const string& guid, const string &witness)
{
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	string offer = find_value(r.get_obj(), "offer").get_str();
	int nQty = find_value(r.get_obj(), "quantity").get_int();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offer));
	string currency = find_value(r.get_obj(), "currency").get_str();
	int nQtyOfferBefore = find_value(r.get_obj(), "quantity").get_int();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	string redeemScriptStr = find_value(r.get_obj(), "redeem_script").get_str();
	// this step must be done in the UI, to ensure that the 'total_without_fee' parameter in escrownew is the right price according to the offer_price value converted into the offer currency
	// since the core doesn't know the rate conversions this must be done externally, the seller/buyer/arbiter should check prior to signing escrow transactions.
	CAmount nodeTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	nodeTotal = nodeTotal / pegRates[currency];
	BOOST_CHECK(abs(AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "offer_price").get_real()*nQty)) - nodeTotal) <= 0.1*COIN);


	bool bBuyNow = find_value(r.get_obj(), "buynow").get_bool();
	if (!bBuyNow) {
		const UniValue &escrowBidBefore = EscrowBidFilterFromGUID(node, guid);
		BOOST_CHECK(!escrowBidBefore.empty());
	}
	string escrowaddress = find_value(r.get_obj(), "escrowaddress").get_str();
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "getaddressutxos \"{\\\"addresses\\\": [\\\"" + escrowaddress + "\\\"]}\""));
	UniValue addressUTXOsArray = r.get_array();
	// "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0,\\\"satoshis\\\":10000}]\"
	string inputStr = "\"[";
	for (unsigned int i = 0; i < addressUTXOsArray.size(); i++)
	{
		if (i > 0)
			inputStr += ",";
		const UniValue& utxoObj = addressUTXOsArray[i].get_obj();
		const string& txidStr = find_value(utxoObj.get_obj(), "txid").get_str();
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		CAmount satoshis = find_value(utxoObj.get_obj(), "satoshis").get_int64();
		inputStr += "{\\\"txid\\\":\\\"" + txidStr + "\\\",\\\"vout\\\":" + boost::lexical_cast<string>(nOut) + ",\\\"satoshis\\\":" + boost::lexical_cast<string>(satoshis) + "}";
	}
	inputStr += "]\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasaddscript " + redeemScriptStr));

	// "escrowcreaterawtransaction <type> <escrow guid> <user role> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]>\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowcreaterawtransaction refund " + guid + " " + inputStr + " " + role));
	const UniValue &arr = r.get_array();
	string rawtx = arr[0].get_str();
	BOOST_CHECK(AmountFromValue(arr[1]) >= 0);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "signrawtransaction " + rawtx));
	const UniValue& hex_value = find_value(r.get_obj(), "hex");
	BOOST_CHECK(hex_value.get_str() != rawtx);
	// "escrowrefund <escrow guid> <user role> <rawtx> [witness]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowrefund " + guid + " " + role + " " + hex_value.get_str() + " " + witness));
	BOOST_CHECK(r.get_array().size() == 1);
	GenerateBlocks(10, node);
	GenerateBlocks(10, node);

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offer));
	int nQtyOfferAfter = find_value(r.get_obj(), "quantity").get_int();
	// refund adds qty
	BOOST_CHECK_EQUAL(nQtyOfferAfter, nQtyOfferBefore+nQty);
	if (!bBuyNow) {
		const UniValue &escrowBidAfter = EscrowBidFilterFromGUID(node, guid);
		BOOST_CHECK(escrowBidAfter.empty());
	}
}
void EscrowClaimRefund(const string& node, const string& guid)
{

	UniValue r, a;

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	string offer = find_value(r.get_obj(), "offer").get_str();
	int nQty = find_value(r.get_obj(), "quantity").get_int();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offer));
	string currency = find_value(r.get_obj(), "currency").get_str();
	string rootselleralias = find_value(r.get_obj(), "offerlink_seller").get_str();
	int nQtyOfferBefore = find_value(r.get_obj(), "quantity").get_int();
	

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	// this step must be done in the UI, to ensure that the 'total_without_fee' parameter in escrownew is the right price according to the offer_price value converted into the offer currency
	// since the core doesn't know the rate conversions this must be done externally, the seller/buyer/arbiter should check prior to signing escrow transactions.
	CAmount nodeTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	nodeTotal = nodeTotal / pegRates[currency];
	BOOST_CHECK(abs(AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "offer_price").get_real()*nQty)) - nodeTotal) <= 0.1*COIN);
	string redeemScriptStr = find_value(r.get_obj(), "redeem_script").get_str();
	bool bBuyNow = find_value(r.get_obj(), "buynow").get_bool();
	if (!bBuyNow) {
		const UniValue &escrowBid = EscrowBidFilterFromGUID(node, guid);
		BOOST_CHECK(escrowBid.empty());
	}
	string selleralias = find_value(r.get_obj(), "seller").get_str();
	string reselleralias = find_value(r.get_obj(), "reseller").get_str();
	string buyeralias = find_value(r.get_obj(), "buyer").get_str();
	string arbiteralias = find_value(r.get_obj(), "arbiter").get_str();
	string witnessalias = find_value(r.get_obj(), "witness").get_str();
	string role = find_value(r.get_obj(), "role").get_str();
	CAmount nTotalWithoutFee = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	CAmount nArbiterFee = AmountFromValue(find_value(r.get_obj(), "arbiterfee"));
	CAmount nNetworkFee = AmountFromValue(find_value(r.get_obj(), "networkfee"));
	CAmount nCommission = AmountFromValue(find_value(r.get_obj(), "commission"));
	CAmount nWitnessFee = AmountFromValue(find_value(r.get_obj(), "witnessfee"));
	CAmount nShipping = AmountFromValue(find_value(r.get_obj(), "shipping"));
	CAmount nDeposit = AmountFromValue(find_value(r.get_obj(), "deposit"));
	string escrowaddress = find_value(r.get_obj(), "escrowaddress").get_str();
	BOOST_CHECK(!buyeralias.empty());

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "getaddressutxos \"{\\\"addresses\\\": [\\\"" + escrowaddress + "\\\"]}\""));
	UniValue addressUTXOsArray = r.get_array();
	// "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0,\\\"satoshis\\\":10000}]\"
	string inputStr = "\"[";
	for (unsigned int i = 0; i < addressUTXOsArray.size(); i++)
	{
		if (i > 0)
			inputStr += ",";
		const UniValue& utxoObj = addressUTXOsArray[i].get_obj();
		const string& txidStr = find_value(utxoObj.get_obj(), "txid").get_str();
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		CAmount satoshis = find_value(utxoObj.get_obj(), "satoshis").get_int64();
		inputStr += "{\\\"txid\\\":\\\"" + txidStr + "\\\",\\\"vout\\\":" + boost::lexical_cast<string>(nOut) + ",\\\"satoshis\\\":" + boost::lexical_cast<string>(satoshis) + "}";
	}
	inputStr += "]\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasaddscript " + redeemScriptStr));
	// get balances before
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + selleralias));
	CAmount balanceSellerBefore = AmountFromValue(find_value(r.get_obj(), "balance"));

	CAmount balanceResellerBefore = 0;
	if (!reselleralias.empty()) {
		BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + reselleralias));
		balanceResellerBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	}

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + buyeralias));
	CAmount balanceBuyerBefore = AmountFromValue(find_value(r.get_obj(), "balance"));

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + arbiteralias));
	CAmount balanceArbiterBefore = AmountFromValue(find_value(r.get_obj(), "balance"));

	CAmount balanceWitnessBefore = 0;
	if (!witnessalias.empty()) {
		BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + witnessalias));
		balanceWitnessBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	}
	// "escrowcreaterawtransaction <type> <escrow guid> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]> [user role]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowcreaterawtransaction refund " + guid + " " + inputStr));
	const UniValue &arr = r.get_array();
	string rawtx = arr[0].get_str();
	BOOST_CHECK(AmountFromValue(arr[1]) >= 0);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "signrawtransaction " + rawtx));
	const UniValue& hex_value = find_value(r.get_obj(), "hex");
	// ensure escrow tx is fully signed
	const UniValue& complete_value = find_value(r.get_obj(), "complete");
	BOOST_CHECK(hex_value.get_str() != rawtx);
	BOOST_CHECK(complete_value.get_bool());
	// ensure that you cannot refund with partially signed tx
	BOOST_CHECK_THROW(CallRPC(node, "escrowcompleterefund " + guid + " " + rawtx), runtime_error);
	BOOST_CHECK_NO_THROW(CallRPC(node, "escrowcompleterefund " + guid + " " + hex_value.get_str()));
	GenerateBlocks(10, node);
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offer));
	int nQtyOfferAfter = find_value(r.get_obj(), "quantity").get_int();
	// claim doesn't touch qty
	BOOST_CHECK_EQUAL(nQtyOfferAfter, nQtyOfferBefore);

	// get balances after
	BOOST_CHECK_NO_THROW(a = CallRPC(node, "aliasbalance " + buyeralias));
	CAmount balanceBuyerAfter = AmountFromValue(find_value(a.get_obj(), "balance"));
	BOOST_CHECK(balanceBuyerBefore != balanceBuyerAfter);

	// get balances after
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + selleralias));
	CAmount balanceSellerAfter = AmountFromValue(find_value(r.get_obj(), "balance"));

	CAmount balanceResellerAfter = 0;
	if (!reselleralias.empty()) {
		BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + reselleralias));
		balanceResellerAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	}

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + arbiteralias));
	CAmount balanceArbiterAfter = AmountFromValue(find_value(r.get_obj(), "balance"));

	CAmount balanceWitnessAfter = 0;
	if (!witnessalias.empty()) {
		BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + witnessalias));
		balanceWitnessAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	}

	balanceBuyerBefore += (nWitnessFee + nShipping + nDeposit);
	// if buy it now(not auction), we must have paid total, otherwise we only refund some of the fees
	if (bBuyNow)
		balanceBuyerBefore += nTotalWithoutFee;
	BOOST_CHECK(role == "arbiter" || role == "seller");
	// if seller refunds it, buyer should get arbiter fee back
	if (role == "seller")
		balanceBuyerBefore += nArbiterFee;
	// else arbiter should get the fee
	else if (role == "arbiter")
		balanceArbiterBefore += nArbiterFee;


	BOOST_CHECK(abs(balanceSellerAfter - balanceSellerBefore) <= 0.1*COIN);
	BOOST_CHECK(abs(balanceResellerAfter - balanceResellerBefore) <= 0.1*COIN);
	BOOST_CHECK(abs(balanceBuyerAfter - balanceBuyerBefore) <= 0.1*COIN);
	BOOST_CHECK(abs(balanceArbiterAfter - balanceArbiterBefore) <= 0.1*COIN);
	BOOST_CHECK(abs(balanceWitnessAfter - balanceWitnessBefore) <= 0.1*COIN);

}

const UniValue FindFeedback(const string& node, const string& txid)
{
	UniValue r, ret;
	string query = "\"{\\\"_id\\\":\\\"" + txid + "\\\"}\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "syscoinquery feedback " + query));
	BOOST_CHECK(r.type() == UniValue::VARR);
	const UniValue &arrayValue = r.get_array();
	BOOST_CHECK(ret.read(arrayValue[0].get_str()));
	BOOST_CHECK(!ret.isNull());
	return ret;
}
void EscrowClaimRelease(const string& node, const string& guid)
{
	UniValue r;
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	string offer = find_value(r.get_obj(), "offer").get_str();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offer));
	string currency = find_value(r.get_obj(), "currency").get_str();
	float fOfferPrice = find_value(r.get_obj(), "price").get_real();
	int icommission = find_value(r.get_obj(), "commission").get_int();
	string sellerlink_alias = find_value(r.get_obj(), "offerlink_seller").get_str();
	string offertype = find_value(r.get_obj(), "offertype").get_str();
	int nQtyOfferBefore = find_value(r.get_obj(), "quantity").get_int();

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowinfo " + guid));
	string redeemScriptStr = find_value(r.get_obj(), "redeem_script").get_str();
	string selleralias = find_value(r.get_obj(), "seller").get_str();
	string reselleralias = find_value(r.get_obj(), "reseller").get_str(); 
	string buyeralias = find_value(r.get_obj(), "buyer").get_str();
	string arbiteralias = find_value(r.get_obj(), "arbiter").get_str();
	string witnessalias = find_value(r.get_obj(), "witness").get_str();
	string role = find_value(r.get_obj(), "role").get_str();
	int nQty = find_value(r.get_obj(), "quantity").get_int();
	CAmount nTotalWithoutFee = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	CAmount nArbiterFee = AmountFromValue(find_value(r.get_obj(), "arbiterfee"));
	CAmount nNetworkFee = AmountFromValue(find_value(r.get_obj(), "networkfee"));
	CAmount nCommission = AmountFromValue(find_value(r.get_obj(), "commission"));
	CAmount nWitnessFee = AmountFromValue(find_value(r.get_obj(), "witnessfee"));
	CAmount nShipping = AmountFromValue(find_value(r.get_obj(), "shipping"));
	CAmount nDeposit = AmountFromValue(find_value(r.get_obj(), "deposit"));

	string escrowaddress = find_value(r.get_obj(), "escrowaddress").get_str();
	BOOST_CHECK(!selleralias.empty());
	

	// this step must be done in the UI, to ensure that the 'total_without_fee' parameter in escrownew is the right price according to the offer_price value converted into the offer currency
	// since the core doesn't know the rate conversions this must be done externally, the seller/buyer/arbiter should check prior to signing escrow transactions.
	CAmount nodeTotal = AmountFromValue(find_value(r.get_obj(), "total_without_fee"));
	nodeTotal = nodeTotal / pegRates[currency];
	BOOST_CHECK(abs(AmountFromValue(strprintf("%.*f", 8, find_value(r.get_obj(), "offer_price").get_real()*nQty)) - nodeTotal) <= 0.1*COIN);
	BOOST_CHECK(pegRates.count(currency) > 0 && pegRates[currency] > 0);
	CAmount offerprice = AmountFromValue(strprintf("%.*f", 8, fOfferPrice * pegRates[currency]));
	CAmount nTotalOfferPrice = offerprice*nQty;
	
	int discount = 0;
	// this step must be done in the UI,
	// check to ensure commission is correct
	if (!sellerlink_alias.empty())
	{
		selleralias = sellerlink_alias;
		discount = FindAliasDiscount("node1", selleralias, buyeralias);
	}
	CAmount nCommissionCompare = 0;
	int markup = discount + icommission;
	if (markup > 0)
		nCommissionCompare = nTotalOfferPrice*(markup / 100);

	BOOST_CHECK_EQUAL(nCommission , nCommissionCompare);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "getaddressutxos \"{\\\"addresses\\\": [\\\"" + escrowaddress + "\\\"]}\""));
	UniValue addressUTXOsArray = r.get_array();
	// "\"[{\\\"txid\\\":\\\"myid\\\",\\\"vout\\\":0,\\\"satoshis\\\":10000}]\"
	string inputStr = "\"[";
	for (unsigned int i = 0; i < addressUTXOsArray.size(); i++)
	{
		if (i > 0)
			inputStr += ",";
		const UniValue& utxoObj = addressUTXOsArray[i].get_obj();
		const string& txidStr = find_value(utxoObj.get_obj(), "txid").get_str();
		const int& nOut = find_value(utxoObj.get_obj(), "outputIndex").get_int();
		CAmount satoshis = find_value(utxoObj.get_obj(), "satoshis").get_int64();
		inputStr += "{\\\"txid\\\":\\\"" + txidStr + "\\\",\\\"vout\\\":" + boost::lexical_cast<string>(nOut) + ",\\\"satoshis\\\":" + boost::lexical_cast<string>(satoshis) + "}";
	}
	inputStr += "]\"";
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasaddscript " + redeemScriptStr));
	// get balances before
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + selleralias));
	CAmount balanceSellerBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	CAmount balanceResellerBefore = 0;
	if (!reselleralias.empty()) {
		BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + reselleralias));
		balanceResellerBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	}
	
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + buyeralias));
	CAmount balanceBuyerBefore = AmountFromValue(find_value(r.get_obj(), "balance"));

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + arbiteralias));
	CAmount balanceArbiterBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	CAmount balanceWitnessBefore = 0;
	if (!witnessalias.empty()) {
		BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + witnessalias));
		balanceWitnessBefore = AmountFromValue(find_value(r.get_obj(), "balance"));
	}
	// "escrowcreaterawtransaction <type> <escrow guid> <[{\"txid\":\"id\",\"vout\":n, \"satoshis\":n},...]> [user role]\n"
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "escrowcreaterawtransaction release " + guid + " " + inputStr));
	const UniValue &arr = r.get_array();
	string rawtx = arr[0].get_str();
	BOOST_CHECK(AmountFromValue(arr[1]) >= 0);
	// rawtx should be partially signed already, now complete the signing process
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "signrawtransaction " + rawtx));
	const UniValue& hex_value = find_value(r.get_obj(), "hex");
	// ensure escrow tx is fully signed
	const UniValue& complete_value = find_value(r.get_obj(), "complete");
	BOOST_CHECK(hex_value.get_str() != rawtx);
	BOOST_CHECK(complete_value.get_bool());
	// ensure that you cannot release with partially signed tx
	BOOST_CHECK_THROW(CallRPC(node, "escrowcompleterelease " + guid + " " + rawtx), runtime_error);
	BOOST_CHECK_NO_THROW(CallRPC(node, "escrowcompleterelease " + guid + " " + hex_value.get_str()));
	GenerateBlocks(10, node);
	GenerateBlocks(10, node);
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "offerinfo " + offer));
	int nQtyOfferAfter = find_value(r.get_obj(), "quantity").get_int();
	// we have already changed qty as we ack the escrow when calling escrownew in this test suite unless its an auction(we are not acking auction bids or buynow purchases in our test suite)
	if (offertype != "AUCTION") {
		BOOST_CHECK_EQUAL(nQtyOfferBefore, nQtyOfferAfter);
	}
	else
		BOOST_CHECK_EQUAL(nQtyOfferBefore-nQty, nQtyOfferAfter);

	// get balances after
	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + selleralias));
	CAmount balanceSellerAfter = AmountFromValue(find_value(r.get_obj(), "balance"));

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + buyeralias));
	CAmount balanceBuyerAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	CAmount balanceResellerAfter = 0;
	if (!reselleralias.empty()) {
		BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + reselleralias));
		balanceResellerAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	}

	BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + arbiteralias));
	CAmount balanceArbiterAfter = AmountFromValue(find_value(r.get_obj(), "balance"));

	CAmount balanceWitnessAfter = 0;
	if (!witnessalias.empty()) {
		BOOST_CHECK_NO_THROW(r = CallRPC(node, "aliasbalance " + witnessalias));
		balanceWitnessAfter = AmountFromValue(find_value(r.get_obj(), "balance"));
	}

	balanceSellerBefore += (nTotalWithoutFee - nCommission);
	BOOST_CHECK(role == "arbiter" || role == "buyer");
	// if buyer released it, he should get arbiter fee back
	if (role == "buyer")
		balanceBuyerBefore += nArbiterFee;
	// else arbiter should get the fee
	else if(role == "arbiter")
		balanceArbiterBefore += nArbiterFee;

	balanceResellerBefore += nCommission;
	balanceWitnessBefore += nWitnessFee;

	BOOST_CHECK(abs(balanceSellerAfter - balanceSellerBefore) <= 0.1*COIN);
	BOOST_CHECK(abs(balanceResellerAfter - balanceResellerBefore) <= 0.1*COIN);
	BOOST_CHECK(abs(balanceBuyerAfter - balanceBuyerBefore) <= 0.1*COIN);
	BOOST_CHECK(abs(balanceArbiterAfter - balanceArbiterBefore) <= 0.1*COIN);
	BOOST_CHECK(abs(balanceWitnessAfter - balanceWitnessBefore) <= 0.1*COIN);

}
BasicSyscoinTestingSetup::BasicSyscoinTestingSetup()
{
}
BasicSyscoinTestingSetup::~BasicSyscoinTestingSetup()
{
}
SyscoinTestingSetup::SyscoinTestingSetup()
{
	StartNodes();
	// rate converstion to SYS
	pegRates["USD"] = 2690.1;
	pegRates["EUR"] = 2695.2;
	pegRates["GBP"] = 2697.3;
	pegRates["CAD"] = 2698.0;
	pegRates["BTC"] = 100000.0;
	pegRates["ZEC"] = 10000.0;
	pegRates["SYS"] = 1.0;
}
SyscoinTestingSetup::~SyscoinTestingSetup()
{
	StopNodes();
}
SyscoinMainNetSetup::SyscoinMainNetSetup()
{
	StartMainNetNodes();
}
SyscoinMainNetSetup::~SyscoinMainNetSetup()
{
	StopMainNetNodes();
}
