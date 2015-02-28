#define ERROFPARA               "20004001"    //para is err;
#define ERROFSTAT               "20004002"    //fail to stat a file or directory;
#define ERROFOPENDIR            "20004003"    //fail to opendir;
#define ERROFOPEN               "20004004"    //fail to open file;
#define ERROFMALLOC             "20004005"    //fail to Malloc() calloc();

#define ERROFHASHCHECK          "20004010"

#define ERROFDELDCP             "20004020"    //delete dcp failed;
#define ERROFRMISDOING          "20004021"    //rm task is doing now;

#define ERROFCPISDOING          "20004030"    //copy tast is doing now;
#define ERROFPARSEXML           "20004031"    //parse the xmlpara is failed when check cpDcp;
#define ERROFFILEEXIST          "20004032"    //the copy file has exist;
#define ERROFSTOPCP             "20004033"    //there is no cp task when stop command;
#define ERROFCPRSTOP            "20004034"    //the cp task is stop by signal when get cpstat;
#define ERROFCPWDES             "20004035"    //write des file is failed when copy;
#define ERROFCPRSRC             "20004036"    //read src file is failed when copy;
#define ERROFGETCPSTAT          "20004037"    //get cp statu is failed;
#define ERROFNOCPTASK           "20004038"    //there is no cp task when geting cp statu;
#define ERROFCPISGOBACK         "20004039"    //when cp is failed, the cp pthread is going back;
#define ERROFGETDIRECT          "20004040"    //get directory is failed;
#define ERROFMOVE               "20004041"    //when call mv_file(),the err is occur;

#define ERROFCREATDCPTREE       "20004050"    //creat DCP tree is failed when get DCP xml;
#define ERROFCREATKDMTREE       "20004051"    //creat KDM tree is failed when get KDM xml;

#define ERROFDCPSYNC            "20004060"    //dcp sync is failed;

#define ERROFNETCONNECTION      "20004098"
#define ERROFSERVERNOTREPLY     "20004099"
#define ERROFUNKNOWN            "20004100"

#define ERROFNOFREESPACE        "20003097"    //used when copy dcp;

#define SUCC "00000000"
#define LEN_RET 8 
