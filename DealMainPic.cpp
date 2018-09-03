#include "ai_defs.h"
#include "alg_public_ivsctrl.h"
#include "define.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
using namespace std;
#include <cstdlib>  
#include <dlfcn.h> 
#include <sys/timeb.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <occi.h>
#include <unistd.h>
#include <sstream>
#include <stdexcept>
using namespace oracle::occi; 

#define DWORD unsigned long

typedef long (*PFN_TEST)(const char* szName, int nAge);
typedef int (*PVDAnalyseOpen)(void **ppvHandle, void *pvParam);
typedef int (*PVDAnalyseClose)(void *pvHandle);
typedef int (*PVDAnalyseProcess)(void *pvHandle, void* pvInput, void* pvOutput);
PFN_TEST g_Test = NULL;

string g_username = "";  
string g_userpwd = "";  
string g_srvName = "";
string g_uploadPath = "";
string g_ffmpegPath = "";

typedef struct tagVedioWarn {
	std::string sYwid;
	std::string sFileName;
	int iFrameWidth;
	int iFrameHeight;
	int iFrameCount;
	int iFrameDelay;

} VEDIOWARN;

int write_log (char *pLogStr) 
{  
	//ReadIniValue("LOG","WriteLog",cIsLog);
	//if(0!=strcmp(cIsLog,"true"))
	//{			
	//	return 0;
	//}

	char curTime[100] = {0};
	int ret = -1; 

	ofstream outf;
	outf.open("LocalLog.txt",ios::app);
	outf<<pLogStr<<endl;
	outf.close(); 
    return 0;  
} 


void GetConfigInfo()
{

   ifstream configFile;
   string path = "Info.conf";
   configFile.open(path.c_str());
   string strLine;
   string filepath;
   if(configFile.is_open())
    {
      while (!configFile.eof())
        {
            getline(configFile, strLine);
            size_t pos = strLine.find('=');
            string key = strLine.substr(0, pos);
                    
            if(key == "UploadPath")
            	{
                g_uploadPath = strLine.substr(pos + 1);
		    //cout<<g_uploadPath<<endl;            
            	}
		else if(key == "FfmpegPath")
		{
		    g_ffmpegPath = strLine.substr(pos + 1);
		}
		else if(key == "DbServerName")
            	{
                g_srvName = strLine.substr(pos + 1);
 		    //cout<<g_srvName<<endl;            
            	}
		else if(key == "DbUsrName")
            	{
                g_username = strLine.substr(pos + 1);            
            	}
		else if(key == "DbUsrPwd")
            	{
                g_userpwd = strLine.substr(pos + 1);            
            	}
	}
    }
   else
    {
        cout << "Cannot open config file!" << endl;
    }



}

int GetVedioWarnInfo(VEDIOWARN *pVedioWarn)
{
  Environment *env=Environment::createEnvironment();  
 
  int iFlag = 0; 
  
  try  
   {  
    Connection *conn = env->createConnection(g_username, g_userpwd, g_srvName);   
  
	  // 数据操作,创建Statement对象  
        Statement *pStmt = NULL;    // Statement对象  
        pStmt = conn->createStatement();  
        if(NULL == pStmt) {  
                                printf("createStatement error.\n");  
                                return -1;  
                          }  
  
	   // 查询数据库时间  
        std::string strTemp;
	  std::string strGuid;  
        ResultSet *pRs = pStmt->executeQuery("select \"vedioID\",\"fileName\",\"frameWidth\",\"frameHeight\",\"frameCount\",\"frameDelay\" from (select * from T_ZNCT_VEDIO_WARN where \"result\" = 0 order by \"uploadTime\" )C where rownum=1");  
        while(pRs->next()) {  
                              pVedioWarn->sYwid = pRs->getString(1);
					pVedioWarn->sFileName = pRs->getString(2);
					pVedioWarn->iFrameWidth = pRs->getInt(3);
					pVedioWarn->iFrameHeight = pRs->getInt(4);
					pVedioWarn->iFrameCount = pRs->getInt(5);
					pVedioWarn->iFrameDelay = pRs->getInt(6);  
					iFlag = 1;
                              break;  
                           }  
        pStmt->closeResultSet(pRs);	  
  
           // 终止Statement对象  
        conn->terminateStatement(pStmt);  
  
  
    env->terminateConnection(conn);  
  }  
  catch(SQLException e)  
  {  
    cout<<e.what()<<endl;  
  }  
  
  Environment::terminateEnvironment(env);  
  //cout<<"end!"<<endl;  
  return iFlag; 

}
int UpdateVedioWarnInfo(VEDIOWARN *pVedioWarn)
{
Environment *env=Environment::createEnvironment();  
 
  int iFlag = 0; 
  
  try  
   {  
    Connection *conn = env->createConnection(g_username, g_userpwd, g_srvName);   
  
	  // 数据操作,创建Statement对象  
        Statement *pStmt = NULL;    // Statement对象  
        pStmt = conn->createStatement();  
        if(NULL == pStmt) {  
                                printf("createStatement error.\n");  
                                return -1;  
                          }  
  


	pStmt->setAutoCommit(TRUE);
  	char strUpdateDB[200] = {0};
  	sprintf(strUpdateDB, ("update T_ZNCT_VEDIO_WARN set \"result\" = 1,\"dealTime\" = sysdate where \"vedioID\" = \'%s\'"), pVedioWarn->sYwid.c_str());
  	cout<<strUpdateDB<<endl;
  	pStmt->setSQL(strUpdateDB);
   	// 执行SQL语句  
	unsigned int nRet = pStmt->executeUpdate();  
	if(nRet == 0) {  
    	printf("executeUpdate insert error.\n");  
   	} 


   
  
           // 终止Statement对象  
        conn->terminateStatement(pStmt);  
  
  
    env->terminateConnection(conn);  
  }  
  catch(SQLException e)  
  {  
    cout<<e.what()<<endl;  
  }  
  
  Environment::terminateEnvironment(env);  
  //cout<<"end!"<<endl;  
  return iFlag; 
}
int InsertWarnInfo(string strYwid,int iAlertType,int iAlertFrame,int iFrameDelay)
{
  Environment *env=Environment::createEnvironment();  
  int iFlag = 0; 
  
  try  
   {  
    	  Connection *conn = env->createConnection(g_username, g_userpwd, g_srvName);   
  
          // 数据操作,创建Statement对象  
        Statement *pStmt = NULL;    // Statement对象  
        pStmt = conn->createStatement();  
        if(NULL == pStmt) {  
                                printf("createStatement error.\n");  
                                return -1;  
                          }  
  
	   // 查询数据库时间  
        std::string strTemp;
	  std::string strGuid;
	  pStmt->setAutoCommit(TRUE);
	  char strInsertDB[200] = {0};
		int alertSecond = iAlertFrame/iFrameDelay;
		char alertTime[20];
		sprintf(alertTime,"%.2d:%.2d",alertSecond/60,alertSecond%60);
	  sprintf(strInsertDB, ("insert into T_ZNCT_WARN_INFO select sys_guid(),\'%s\',%d,%d,sysdate,\'%s\',%d from dual"), strYwid.c_str(),iAlertType,iAlertFrame,alertTime,alertSecond);
	  cout<<strInsertDB<<endl;
	  pStmt->setSQL(strInsertDB);
	   // 执行SQL语句  
        unsigned int nRet = pStmt->executeUpdate();  
        if(nRet == 0) {  
            printf("executeUpdate insert error.\n");  
           }   
        
        conn->terminateStatement(pStmt);  
    	  env->terminateConnection(conn);  
  }  
  catch(SQLException e)  
  {  
    cout<<e.what()<<endl;  
  }  
  
  Environment::terminateEnvironment(env);  
  //cout<<"end!"<<endl;  
  return iFlag; 

}

int UpdateWarnInfo(string strYwid)
{
  Environment *env=Environment::createEnvironment();  
  int iFlag = 0; 
  
  try  
   {  
    	  Connection *conn = env->createConnection(g_username, g_userpwd, g_srvName);   
  
	   // 数据操作,创建Statement对象  
        Statement *pStmt = NULL;    // Statement对象  
        pStmt = conn->createStatement();  
        if(NULL == pStmt) {  
                                printf("createStatement error.\n");  
                                return -1;  
                          }  
  
           // 查询数据库时间  
        std::string strTemp;
	  std::string strGuid;
	  pStmt->setAutoCommit(TRUE);
	  char strUpdateDB[200] = {0};
	  sprintf(strUpdateDB, ("update T_ZNCT_VEDIO_WARN set \"result\" = 2,\"dealTime\" = sysdate where \"vedioID\" = \'%s\'"), strYwid.c_str());
	  cout<<strUpdateDB<<endl;
	  pStmt->setSQL(strUpdateDB);
	   // 执行SQL语句  
        unsigned int nRet = pStmt->executeUpdate();  
        if(nRet == 0) {  
            printf("executeUpdate insert error.\n");  
           }   
        
        conn->terminateStatement(pStmt);  
    	  env->terminateConnection(conn);  
  }  
  catch(SQLException e)  
  {  
    cout<<e.what()<<endl;  
  }  
  
  Environment::terminateEnvironment(env);  
  //cout<<"end!"<<endl;  
  return iFlag; 

}

string GetXmlName(int iWarnType)
{
	string strXMLName;
	strXMLName = "";
	if(iWarnType == 1)
	{
		strXMLName = "lineAlert";
	}
	else if(iWarnType == 2)
	{
		strXMLName = "sportAlert";

	}
	else if(iWarnType == 3)
	{
		strXMLName = "areaAlert";

	}
	else if(iWarnType == 4)
	{
		strXMLName = "oneDutyAlert"; 
	}
	else if(iWarnType == 5)
	{
		strXMLName = "standUpAlert";

	}
	else if(iWarnType == 6)
	{
		strXMLName = "vedioDealAlert";

	}
	return strXMLName;
}
bool CheckFileExist(std::string filename)
{
	ifstream f(filename.c_str());
	return f.good();
}
std::string exec(const char* cmd) {
	char buffer[128];
	std::string result = "";
	FILE* pipe = popen(cmd, "r");
	if (!pipe) throw std::runtime_error("popen() failed!");
	try {
		while (!feof(pipe)) {
			if (fgets(buffer, 128, pipe) != NULL)
				result += buffer;
		}
	}
	catch (...) {
		pclose(pipe);
		throw;
	}
	pclose(pipe);
	return result;
}
int getVideoDuration(VEDIOWARN *pVideoWarn)
{
	char cTransYuv[300] = { 0 };

	sprintf(cTransYuv, ("%s/ffprobe  -v error -select_streams v:0 -show_entries stream=duration -of default=noprint_wrappers=1:nokey=1 %s/%s"), g_ffmpegPath.c_str(), g_uploadPath.c_str(), pVideoWarn->sFileName.c_str());
	string sExeTranYuv = cTransYuv;
	cout << "duration video:" << sExeTranYuv << endl;
	//return system(sExeTranYuv.c_str());
	return atoi((char*)exec(sExeTranYuv.c_str()).c_str());
}
std::string getSplit(string sentence,int index)
{
	istringstream iss(sentence);
	std::vector<std::string> tokens;
	std::string token;
	while (std::getline(iss, token, '.')) {
		if (!token.empty())
			tokens.push_back(token);
	}
	return tokens[index];
}
int SplitVideo(VEDIOWARN *pVideoWarn,int start,int duration,int nameIndex)
{
	char cTransYuv[300] = { 0 };

	sprintf(cTransYuv, ("%s/ffmpeg  -i %s/%s -ss %d -t %d -acodec copy -vcodec copy %s/%s_%d.%s"), g_ffmpegPath.c_str(), g_uploadPath.c_str(), pVideoWarn->sFileName.c_str(),start,duration, g_uploadPath.c_str(), pVideoWarn->sYwid.c_str(),nameIndex, getSplit(pVideoWarn->sFileName.c_str(),1).c_str());
	string sExeTranYuv = cTransYuv;
	cout << "split video:" << sExeTranYuv << endl;
	return system(sExeTranYuv.c_str());
}
void ConvertToYUV(std::string sFileName)
{
	char cTransYuv[300] = { 0 };
	sprintf(cTransYuv, ("%s/ffmpeg -y -i  %s/%s %s/%s.yuv"), g_ffmpegPath.c_str(), g_uploadPath.c_str(), sFileName.c_str(), g_uploadPath.c_str(), getSplit(sFileName.c_str(),0).c_str());
	string sExeTranYuv = cTransYuv;
	cout << "convert to yuv:" << sExeTranYuv << endl;
	system(sExeTranYuv.c_str());
}
int getFrameByName(std::string sFileName)
{
	char cTransYuv[300] = { 0 };

	sprintf(cTransYuv, ("%s/ffprobe -v error -count_frames -select_streams v:0 -show_entries stream=nb_frames -of default=nokey=1:noprint_wrappers=1  %s/%s"), g_ffmpegPath.c_str(), g_uploadPath.c_str(), sFileName.c_str());
	string sExeTranYuv = cTransYuv;
	cout << "duration video:" << sExeTranYuv << endl;
	return atoi((char*)exec(sExeTranYuv.c_str()).c_str());
}
void SingleOnePic(VEDIOWARN *pVideoWarn, std::string fileInputName,int index, int total,int iWarnType, void *pHandle)
{
	int iWidth = pVideoWarn->iFrameWidth;
	int iHeight = pVideoWarn->iFrameHeight;
	
}
int dectectVideo(VEDIOWARN *pVideoWarn,std::string fileInputName,int total)
{
	int iWidth = pVideoWarn->iFrameWidth;
	int iHeight = pVideoWarn->iFrameHeight;
	void* handle = dlopen("./libivsctrl_x86_linux.so", RTLD_LAZY);
	if (!handle)
	{
		printf("ERROR, Message(%s).\n", dlerror());
		return -1;
	}

	for (int iWarnType = 1; iWarnType<6; iWarnType++)
	{

		TAnalyseParam struAnalyseParam;
		memset(&struAnalyseParam, 0, sizeof(TAnalyseParam));
		struAnalyseParam.ps8ParamData = new char[8000];
		PVDAnalyseOpen g_VDAnalyseOpen = NULL;
		PVDAnalyseProcess g_VDAnalyseProcess = NULL;
		PVDAnalyseClose g_VDAnalyseClose = NULL;
		void *pHandle = NULL;
		FILE *fp;
		char *buf;
		char fileName[200] = { 0 };
		std::string strXmlName = GetXmlName(iWarnType);
		sprintf(fileName, ("%s/%s_%s.txt"), g_uploadPath.c_str(), pVideoWarn->sYwid.c_str(), strXmlName.c_str());
		cout << "显示分析模块XML:" << fileName << endl;

		if ((fp = fopen(fileName, "rb")) == NULL)
		{
			cout << "分析模块XML文件未找到！" << fileName << endl;
			continue;
		}

		buf = new char[8000];
		fread(buf, 1, 8000, fp);

		strcpy(struAnalyseParam.ps8ParamData, buf);
		struAnalyseParam.u32ParamLen = strlen(buf);


		//创建一个分析模块
		g_VDAnalyseOpen = (PVDAnalyseOpen)dlsym(handle, "VDAnalyseOpen");
		char* szError = dlerror();
		if (szError != NULL)
		{
			printf("ERROR, Message(%s).\n", szError);
			dlclose(handle);
			//return -1;
			continue;
		}
		if (g_VDAnalyseOpen != NULL)
		{
			int iOk = g_VDAnalyseOpen(&pHandle, &struAnalyseParam);
		}
		if (pHandle == NULL)
		{
			printf("g_VDAnalyseOpen(&pHandle,&struAnalyseParam) : pHandle is null.\n");

			continue;
		}

		//获得分析处理函数

		g_VDAnalyseProcess = (PVDAnalyseProcess)dlsym(handle, "VDAnalyseProcess");
		szError = dlerror();
		if (szError != NULL)
		{
			printf("ERROR, Message(%s).\n", szError);
			dlclose(handle);
			//return -1;
			continue;
		}
		else
		{
			printf("VDAnalyseProcess.\n");
		}
		for (int index = 0; index < total; index++) {
			char name[50];
			sprintf(name, "%s/%s_%d.%s", g_uploadPath.c_str(), pVideoWarn->sYwid.c_str(), index, "yuv");
			cout << "name:" << name << endl;
			
			//分析一帧图像


			unsigned char *YUV1 = NULL;
			YUV1 = new unsigned char[iWidth * iHeight * 3 / 2];
			int bufLen = iWidth * iHeight * 3 / 2;
			FILE* pFileIn;

			if ((pFileIn = fopen(fileInputName.c_str(), "rb")) == NULL) //if((pFileIn=fopen(fileInputName,"rb"))==NULL)
			{
				cout << "YUV文件打开失败！";
				//DeleteFile
				/*for (int m = 1; m<7; m++)
				{
				char fileName[200] = { 0 };
				std::string strXmlName = GetXmlName(m);
				sprintf(fileName, ("%s/%s_%s.txt"), g_uploadPath.c_str(), pVideoWarn->sYwid.c_str(), strXmlName.c_str());
				remove(fileName);

				}*/
				//char fileInputName[200] = {0};
				//sprintf(fileInputName, ("%s/%s.yuv"),g_uploadPath.c_str(),pVideoWarn->sYwid.c_str());
				//remove(fileInputName.c_str());

			}
			else {
				printf("fopen.\n");
			}

			u32 dwTimeStamp = 0;
			printf("pVideoWarn->sFileName= %s pVideoWarn->iFrameCount= %d.\n", pVideoWarn->sFileName.c_str(), pVideoWarn->iFrameCount);
			//int div = getFrameByName(pVideoWarn->sFileName)/total;
			int div = pVideoWarn->iFrameCount*pVideoWarn->iFrameDelay / total;
			int start = div * index;
			int end = 0;
			if (div*(index + 1) > pVideoWarn->iFrameCount*pVideoWarn->iFrameDelay)
			{
				end = pVideoWarn->iFrameCount*pVideoWarn->iFrameDelay;
			}
			else
			{
				end = div * (index + 1);
			}
			char sstart[10] = { 0 }; char send[10] = { 0 };
			sprintf(sstart, "%d", start);
			sprintf(send, "%d", end);
			cout << "start:" << sstart << endl;
			cout << "end:" << send << endl;
			for (int i = start; i<end; i++)
			{

				TAnalyseInput struAnalyseInput = { 0 };
				TAnalyseOutput struAnalyseOutput = { 0 };
				dwTimeStamp += pVideoWarn->iFrameDelay;
				struAnalyseInput.u64TimeStamp = dwTimeStamp;

				fread(YUV1, bufLen * sizeof(unsigned char), 1, pFileIn);
				TImage tempImage = { 0 };
				tempImage.u32Type = AI_I420;
				tempImage.atPlane[0].l32Width = iWidth;
				tempImage.atPlane[0].l32Height = iHeight;
				tempImage.atPlane[0].l32Stride = iWidth;
				tempImage.atPlane[0].pvBuffer = YUV1;

				tempImage.atPlane[1].l32Width = iWidth >> 1;
				tempImage.atPlane[1].l32Height = iHeight >> 1;
				tempImage.atPlane[1].l32Stride = iWidth >> 1;
				tempImage.atPlane[1].pvBuffer = YUV1 + iWidth * iHeight;

				tempImage.atPlane[2].l32Width = iWidth >> 1;
				tempImage.atPlane[2].l32Height = iHeight >> 1;
				tempImage.atPlane[2].l32Stride = iWidth >> 1;
				tempImage.atPlane[2].pvBuffer = YUV1 + iWidth * iHeight * 5 / 4;
				struAnalyseInput.tImgIn = tempImage;



				long iOk = g_VDAnalyseProcess(pHandle, &struAnalyseInput, &struAnalyseOutput);
				cout << "OK:" << iOk << endl;
				if (iOk == 0)
				{
					if (struAnalyseOutput.u32RstNum)
					{
						//u32RstNum 大于0就是有报警
						printf("pAnalyseOutput alert!(Frame:%d,WarnType:%d).\n", i, iWarnType);
						/*
						for(int j=0;j < MAX_RESULT_NUM;j++)
						{
						printf("dwTimeStamp = .\n",struAnalyseOutput.atRstData[j].u64TimeStamp);
						}
						*/
						InsertWarnInfo(pVideoWarn->sYwid, iWarnType, i, pVideoWarn->iFrameDelay);

					}
					else
					{
						//printf("pAnalyseOutput struAnalyseOutput.u32RstNum = %d.\n",struAnalyseOutput.u32RstNum);
					}

				}
				else
				{
					printf("*VDAnalyseProcess Error (%ld).\n", iOk);
				}

			}
			delete[] YUV1;
			YUV1 = NULL;
		}
		g_VDAnalyseClose = (PVDAnalyseClose)dlsym(handle, "VDAnalyseClose");
		szError = dlerror();
		if (szError != NULL)
		{
			printf("ERROR, Message(%s).\n", szError);
			dlclose(handle);
			//return -1;
			continue;
		}
		long iOk = g_VDAnalyseClose(pHandle);
		if (iOk == 0)
		{
			//printf("*VDAnalyseClose Sussess! (%ld).\n", iOk);				

		}
		else
		{
			printf("*VDAnalyseClose Error (%ld).\n", iOk);
		}
		delete[] buf;
		buf = NULL;
		delete[] struAnalyseParam.ps8ParamData;
		struAnalyseParam.ps8ParamData = NULL;
		

	}	

	if (handle)
	{
		dlclose(handle);
	}
	
	remove(fileInputName.c_str());
	

}
int DealVedio()
{
	cout << "begin DealVedio..." << endl;
	VEDIOWARN tempVedioWarn;
	int iFlag = GetVedioWarnInfo(&tempVedioWarn);
	//存在未处理告警信息
	if(iFlag == 1)
	{
		
		char fileInputName[200] = {0};
		char OriFileName[200] = {0};
		char cTransYuv[300] = {0};

		sprintf(OriFileName,("%s/%s"),g_uploadPath.c_str(),tempVedioWarn.sFileName.c_str());
		cout<<"显示原始的视频:"<<OriFileName<<endl;
		if(!CheckFileExist(OriFileName))
		{
			return -1;
		}
		UpdateVedioWarnInfo(&tempVedioWarn);

		int duration = getVideoDuration(&tempVedioWarn);
		int cut = 5;
		int count = duration / cut;
		char scount[10] = { 0 }; char sdur[10] = { 0 };
		sprintf(sdur, "%d", duration);
		sprintf(scount, "%d", count);
		cout << "时长:" << sdur << endl;
		cout << "循环次数:" << scount << endl;
		for (int i = 0; i <= count; i++)
		{
			char name[100];
			if(i==count)
				SplitVideo(&tempVedioWarn, i * cut, duration-i*cut, i);
			else
				SplitVideo(&tempVedioWarn,i*cut,cut,i);
			sprintf(name, "%s_%d.%s", tempVedioWarn.sYwid.c_str(), i, getSplit(tempVedioWarn.sFileName.c_str(),1).c_str());
			cout << "convert yuv:" << name << endl;
			ConvertToYUV(name);
			
			
			
		}
		dectectVideo(&tempVedioWarn, fileInputName,  count);
		for (int i = 0; i <= count; i++)
		{
			char name[100]; char splitFile[100];
			sprintf(name, "%s_%d.%s", tempVedioWarn.sYwid.c_str(), i, getSplit(tempVedioWarn.sFileName.c_str(), 1).c_str());

			sprintf(splitFile, "%s/%s", g_uploadPath.c_str(), name);
			cout << "remove file:" << splitFile << endl;
			remove(splitFile);
			cout << "显示要分析的视频:" << fileInputName << endl;

		}
		
		UpdateWarnInfo(tempVedioWarn.sYwid);
		//DeleteFile
		for (int m = 1; m < 7; m++)
		{
			char fileName[200] = { 0 };
			std::string strXmlName = GetXmlName(m);
			sprintf(fileName, ("%s/%s_%s.txt"), g_uploadPath.c_str(), tempVedioWarn.sYwid.c_str(), strXmlName.c_str());
			remove(fileName);

		}
		
		remove(OriFileName);
		return 1;
		

	}
	else
	{
		cout << "No VedioData to Deal..." << endl;
	}
	cout << "End DealVedio..." << endl;


	return -1;
	
}

void *thread(void *ptr)
{
      while(1) 
	{
        sleep(10);
	  DealVedio();
    	}
    	return 0;
}

int main()
{
	GetConfigInfo();
      pthread_t id;
    	int ret = pthread_create(&id, NULL, thread, NULL);
    	if(ret) {
        cout << "Create pthread error!" << endl;
        return 1;
    	}
	pthread_join(id, NULL);
	return 0 ;

}



 

