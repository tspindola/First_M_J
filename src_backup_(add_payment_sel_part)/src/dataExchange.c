/*------------------------------------------------------------
* FileName: dataExchange.c
* Author: Jeff
* Date: 2017-04-25
------------------------------------------------------------*/
#include "global.h"
#include "dataExchange.h"
#include "Crc32.h"


#define JSON_FILE  				"./data/json/newjason.dat"

#ifdef JEFF_DEBUG
	
#endif

struct miniposToMainapp glMiniPosData;
struct mainappToMinipos glMainAppData;

int glTotalCount;
double glTotalPrice;
double glTotalVat;
char glReverFlag; //reversal flag
char *glCurrencyName = "DKK"; //init current,but get from mainapp
extern double settle_product_vat();
extern double product_node_get_vat(FNODE_T *node);

int GetDataFromNodeStruc(void)
{
	int iProductCount;
	uchar pszformatAmount[30];
	int i;

	glTotalCount = settle_product_count();
	glTotalPrice = settle_product_amount();
	glTotalVat = settle_product_vat();
	glOrderAllProduct.orderQuantity = settle_product_list_node_count();
	glOrderAllProduct.totalPrice = glTotalPrice;
	glOrderAllProduct.totalVat = glTotalVat;
	iProductCount = glOrderAllProduct.orderQuantity;
	Pax_Log(LOG_INFO,"TestiProductCount=%d,iProductTotalCount=%d,iProductTotalAmount=%d",
			iProductCount,glTotalCount,glTotalPrice);
	for(i = 0;i < iProductCount;i++)
	{
		void *node = settle_product_list_node(i);
		glOrderAllProduct.orderLine[i].price = product_node_get_price(node);
		glOrderAllProduct.orderLine[i].product_id = product_node_get_id(node);
		glOrderAllProduct.orderLine[i].quantity = product_node_get_count(node);
		glOrderAllProduct.orderLine[i].vat = product_node_get_vat(node);
		
		strcpy(glOrderAllProduct.orderLine[i].productName,node_get_name(node));
	}
	memset(glMiniPosData.amount,'0',12);
	FormatFloat(glTotalPrice,pszformatAmount);
	memcpy(glMiniPosData.amount + 12 - strlen(pszformatAmount),pszformatAmount,strlen(pszformatAmount));
	return 0;
}


int menu_exec(MENU_SELECTION menuStat){

	int iPayKeyRet = 0;
	int stat = 0;

	stat = menuStat;
	while (stat!=stat_exit) 
	{	
	switch(stat) {	
	case stat_main_menu:
		//Pax_Log(LOG_INFO, "start to entry menu page:_ %s - %d", __FUNCTION__, __LINE__);
		book_menu_process();			
		int ret_mp=smenu_get_ret_value();		
		if (ret_mp==page_code_exit())
		{	
			stat=stat_exit;	
		}			
		if (ret_mp==page_code_ensure()) 
		{	
			stat=stat_settle_menu;
		}
		if(ret_mp == page_code_func())
		{
			stat = stat_exit;
			return START_SHOW_MAINAPP_MENU;
		}
			break;			
	case stat_settle_menu:
		//Pax_Log(LOG_INFO, "start to entry settle page:%s - %d",__FUNCTION__, __LINE__);
		settle_menu_process();			
		int ret_smp=settle_menu_process_get_value();	
		if (ret_smp==page_code_exit())
		{			
			stat=stat_exit;		
		}		
		if (ret_smp==page_code_back())
		{
			stat=stat_main_menu;		
		}		
		if (ret_smp==page_code_ensure()) 
		{			
			stat=stat_exit;
			//stat = stat_payment_menu;
		}			
			break;	
#if 0   //remain it as a show of demo.
	case stat_payment_menu:
		iPayKeyRet = payment_menu_process();
		XuiShowWindow(payment_menu_win(),XUI_HIDE,0);
		if(iPayKeyRet == page_code_back())
		{
			stat = stat_settle_menu;
		}
	    else if(iPayKeyRet == XUI_KEY1 || iPayKeyRet == XUI_KEY2 ||
	      iPayKeyRet == XUI_KEY3)
	     {
			stat=stat_exit;
	      
	      }

	    else if(iPayKeyRet = page_code_timeout_eixt())
	    {
	    	stat=stat_exit;
	    }
		#if 0
		while(0) //fetch and check json data from file
		{
			if (XuiHasKey() && XuiGetKey() == XUI_KEY9)
			{
				break;
			}
		}
		
		#endif
			break;
#endif
	case stat_exit:	
		default:		
			break;	
	};
  }
return 0;
}




int HandleReversal(void)
{
	int iRet;

	if(access(FILE_REVERSALFlAG,F_OK) < 0)
	{
		Pax_Log(LOG_DEBUG,"%s,Line:%d",__FUNCTION__,__LINE__);  //if reversal file is not exist,return 0;
		glReverFlag = '0';
		return 0;
	}
	iRet = ReadFile(FILE_REVERSALFlAG,&glReverFlag,1);
	Pax_Log(LOG_DEBUG,"iRet=%d,glReverFlag=%c,%s,Line:%d",iRet,glReverFlag,__FUNCTION__,__LINE__);
	if(iRet)
	{
		return iRet;
	}
	if(glReverFlag == '1')
	{
		Pax_Log(LOG_DEBUG,"start to process reversal%s,Line:%d",__FUNCTION__,__LINE__);  //if re
		iRet = Request_Process(CMD_UPLOAD_DATA,NORMAL);
		if(iRet)
		{
			return iRet;
		}
	}
	return 0;
}


int SaveMainAppDataForTest(void)  //for test com with MAINAPP
{
#if 1
	int iFd;
	int i;
	int iLen;
	unsigned long ulCRC32;
	unsigned char *pIn;
	unsigned int uiLen;
	unsigned char aucCRC[4];
	char* pTmpCRC32;
	struct mainappToMinipos MainAppData;

	uiLen = sizeof(struct mainappToMinipos);
	memset(&MainAppData,0,uiLen);
	MainAppData.wakeUpReason = MAIN_WAKEUP_STARTUP;
	MainAppData.paymentType = '4';
	strcpy(MainAppData.currency,"DKK");
	//MainAppData.wakeupReason = MAIN_WAKEUP_PAYRESULT;
	pIn = (unsigned char *)&MainAppData;
	crc32Init(&ulCRC32);
	for(i = 0;i < uiLen;i++)
	{
		crc32Update(&ulCRC32,pIn[i]);
	}
	aucCRC[0] = crc32Byte1(&ulCRC32);
	aucCRC[1] = crc32Byte2(&ulCRC32);
	aucCRC[2] = crc32Byte3(&ulCRC32);
	aucCRC[3] = crc32Byte4(&ulCRC32);
	
	remove(FILE_MAINAPP_MINIPOS);
	iFd = open(FILE_MAINAPP_MINIPOS, O_CREAT | O_WRONLY, S_IRWXU|S_IRWXG|S_IRWXO);
	if(iFd < 0)
	{
		Pax_Log(LOG_ERROR, "%s - %d", __FUNCTION__, __LINE__);
		return FILE_ERR_OPEN_FAIL;
	}
	iLen = write(iFd,&MainAppData,uiLen);
	if(iLen != uiLen)
	{
		Pax_Log(LOG_ERROR, "%s - %d", __FUNCTION__, __LINE__);
		close(iFd);
		return FILE_ERR_INVLIDE_DATA;
	}
	iLen = write(iFd, aucCRC, 4);
	if (iLen != 4)
	{
		close(iFd);
		return M2M_WRITE_ERROR;
	}
	close(iFd);
	return M2M_SUCCESS;
#endif


}





int SaveMiniDataForMain(struct miniposToMainapp *pRequest)
{
	unsigned long ulCRC;
	unsigned char aucCRC[4];
	unsigned char *pData = NULL;
	int fd = -1;
	int i = 0;
	int iWritten = 0;

	if (!pRequest)
	{
		return M2M_INVALID_PARAMS;
	}

	pData = (unsigned char *)pRequest;
	crc32Init(&ulCRC);
	for (i = 0; i < sizeof(struct miniposToMainapp); i++)
	{
		crc32Update(&ulCRC, pData[i]);
	}

	aucCRC[0] = crc32Byte1(&ulCRC);
	aucCRC[1] = crc32Byte2(&ulCRC);
	aucCRC[2] = crc32Byte3(&ulCRC);
	aucCRC[3] = crc32Byte4(&ulCRC);


	fd = open(FILE_MINIPOS_MAINAPP,O_RDWR | O_CREAT, (S_IRUSR | S_IWUSR | S_IROTH));
	if (fd < 0)
	{
		return M2M_FILE_ACCESS_DENIED;
	}

	iWritten = write(fd, pData, sizeof(struct miniposToMainapp));
	if (iWritten !=  sizeof(struct miniposToMainapp))
	{
		close(fd);
		return M2M_WRITE_ERROR;
	}

	iWritten = write(fd, aucCRC, 4);
	if (iWritten != 4)
	{
		close(fd);
		return M2M_WRITE_ERROR;
	}

	close (fd);

	return M2M_SUCCESS;
}

int LoadMainDataForMini(struct mainappToMinipos *pRequest)
{
	unsigned long ulCRC;
	unsigned char aucCRC[4], fileCRC[4];
	unsigned char *pData = NULL;
	int fd = -1;
	int i = 0;
	int iRead = 0;

	if (!pRequest)
	{
		return M2M_INVALID_PARAMS;
	}

	if (access(FILE_MAINAPP_MINIPOS, F_OK) != 0)
	{
		return M2M_FILE_NOT_FOUND;
	}

	if (access(FILE_MAINAPP_MINIPOS, R_OK) != 0)
	{
		return M2M_FILE_ACCESS_DENIED;
	}

	pData = (unsigned char *)pRequest;

	// load file data and check crc32
	fd = open(FILE_MAINAPP_MINIPOS, O_RDONLY);
	if (fd < 0)
	{
		return M2M_FILE_ACCESS_DENIED;
	}

	iRead = read(fd, pData, sizeof(struct mainappToMinipos));
	if (iRead != sizeof(struct mainappToMinipos))
	{
		close(fd);
		return M2M_READ_ERROR;
	}

	iRead = read(fd, fileCRC, 4);
	if (iRead != 4)
	{
		close(fd);
		return M2M_READ_ERROR;
	}

	close (fd);

	// check CRC32
	crc32Init(&ulCRC);
	for (i = 0; i < sizeof(struct mainappToMinipos); i++)
	{
		crc32Update(&ulCRC, pData[i]);
	}

	aucCRC[0] = crc32Byte1(&ulCRC);
	aucCRC[1] = crc32Byte2(&ulCRC);
	aucCRC[2] = crc32Byte3(&ulCRC);
	aucCRC[3] = crc32Byte4(&ulCRC);

	if (memcmp(aucCRC, fileCRC, 4) != 0)
	{
		return M2M_INVALID_CRC;
	}
    glCurrencyName = pRequest->currency;
  #ifdef JEFF_DEBUG
  Pax_Log(LOG_DEBUG,"currency=%s,payResult=%c,waiterId=%s,wakeup=%c,orderid=%s,terminalId=%s",
  pRequest->currency,pRequest->paymentResult, pRequest->waiterID,pRequest->wakeUpReason,
  pRequest->orderID,pRequest->terminalID);
  
 
  #endif
	return M2M_SUCCESS;
}


int LoadReverdata(char *pszJsonData)
{
	int iLen;
	int iFileLen;
	int iFd;

	if(access(FILE_REVERSAL, F_OK) < 0) 
	{
		Pax_Log(LOG_ERROR, "%s - %d", __FUNCTION__, __LINE__);
		return FILE_ERR_NOT_EXIST;
	}

	iFileLen = Filesize(FILE_REVERSAL);

	iFd = open(FILE_REVERSAL,O_RDONLY);
	if (iFd < 0) {
		Pax_Log(LOG_ERROR, "%s - %d", __FUNCTION__, __LINE__);
		return FILE_ERR_OPEN_FAIL;
	}
	iLen = read(iFd,pszJsonData,iFileLen);
	if(iLen != iFileLen)
	{
		Pax_Log(LOG_ERROR, "%s - %d", __FUNCTION__, __LINE__);
		close(iFd);
		return FILE_ERR_INVLIDE_DATA;
	}
	close(iFd);
	return OT_OK;
}
	

int ReadJason(unsigned char *pszStr)
{
	int iFd;
	int iFileLen;
	int iFileTemLen;


	if(access(JSON_FILE,F_OK) < 0)
	{
		Pax_Log(LOG_ERROR,"%s,Line:%d",__FUNCTION__,__LINE__);
		return FILE_ERR_NOT_EXIST;
	}

   iFileLen = Filesize(JSON_FILE);

   iFd = open(JSON_FILE, O_RDONLY);
   if(iFd < 0)
   {
	   Pax_Log(LOG_ERROR,"%s,Line:%d",__FUNCTION__,__LINE__);
	   return FILE_ERR_OPEN_FAIL;
   }

    iFileTemLen = read(iFd,pszStr,iFileLen);
    Pax_Log(LOG_ERROR,"iFileTemLen=%d  %s,Line:%d",iFileTemLen,__FUNCTION__,__LINE__);
	if(iFileTemLen != iFileLen)
	{
	   close(iFd);
	   return FILE_ERR_INVLIDE_DATA;
	}
    close(iFd);

	return 0;
}

int SaveUpdateJsonData(char *pszJasonData,int iUpdateJasonFlag)
{
	int iFd;
	int iLen;

	if(iUpdateJasonFlag)
	{
		remove(JSON_FILE);
	}
	iFd = open(JSON_FILE, O_CREAT | O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
	if ( iFd < 0 )
	{
		Pax_Log(LOG_ERROR, "save file fail,iFd=%d %s - %d",iFd, __FUNCTION__, __LINE__);
		return FILE_ERR_OPEN_FAIL;
	}
	

	iLen = write(iFd,pszJasonData, strlen(pszJasonData)); //for safe write
	if(iLen < 0)
	{
		return FILE_ERR_WRITE_FAIL;
	}
	close(iFd);
	return 0;
}


int SaveFile(const char *pszFileName,const void *psData,int iDataLen)
{
	int iFd;
	int iLen;

	Pax_Log(LOG_DEBUG, "saving file:%s,%s - %d",pszFileName, __FUNCTION__, __LINE__);
	iFd = open(pszFileName, O_CREAT | O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO);
	if ( iFd < 0 )
	{
		Pax_Log(LOG_ERROR, "save file fail,iFd=%d %s - %d",iFd, __FUNCTION__, __LINE__);
		return FILE_ERR_OPEN_FAIL;
	}

	iLen = write(iFd,psData, iDataLen); //for safe write
	if(iLen < 0)
	{
		return FILE_ERR_WRITE_FAIL;
	}
	close(iFd);
	return 0;
}

int ReadFile(const char *pszFileName,const void *psData,int iDataLen)
{	
	int iFd;
	int iLen;

	Pax_Log(LOG_DEBUG, "reading file:%s,%s - %d",pszFileName, __FUNCTION__, __LINE__);
	iFd = open(pszFileName,O_RDWR);
	if ( iFd < 0 )
	{
		Pax_Log(LOG_ERROR, "save file:%s_fail,iFd=%d %s - %d",pszFileName,iFd, __FUNCTION__, __LINE__);
		return FILE_ERR_OPEN_FAIL;
	}
	iLen = read(iFd,psData,iDataLen);
	close(iFd);
	if(iLen != iDataLen)
	{
		return 2;
	}
	return 0;
}

int LoadTerminalProduct(void)
{
	int iRet;

	Display_Prompt("PLEASE WAIT", "Processing request...", MSGTYPE_UPLOADING, 0);
	Pax_Log(LOG_INFO,"--------------updating terminal,fun:%s,line:%d",__FUNCTION__,__LINE__);
	iRet = Request_Process(CMD_GET_TXNINFO,UPDATE_JSON);
	HidePromptWin();
	if(iRet)
	{	
		Pax_Log(LOG_DEBUG,"Request_Process,iRet=%d,fun:%s_line:%d",
							iRet,__FUNCTION__,__LINE__);
		return iRet;
	}
	return 0;
}

int UpdateTerminalProduct(void)
{
	int iRet;

	Display_Prompt("PLEASE WAIT", "Processing request...", MSGTYPE_UPLOADING, 0);
	iRet = Request_Process(CMD_GET_TXNINFO,UPDATE_JSON);
	HidePromptWin();
	if(iRet)
	{	
		Pax_Log(LOG_DEBUG,"Request_Process,iRet=%d,fun:%s_line:%d",
							iRet,__FUNCTION__,__LINE__);
		return iRet;
	}
	return 0;
}

static int SaveProductDataMiniposData()
{
	int iRet;
	
	remove(ORDER_AllPRODUCT_FILE);
	iRet = SaveFile(ORDER_AllPRODUCT_FILE,&glOrderAllProduct,sizeof(struct _orderAllProduct));
	if(iRet)
	{	
		Pax_Log(LOG_DEBUG,"save all product_file,iRet=%d,fun:%s_line:%d",
							iRet,__FUNCTION__,__LINE__);
		return iRet;
	}
	glMiniPosData.wakeUpReason = MINI_WAKEUP_STARTPAY;
    iRet = SaveMiniDataForMain(&glMiniPosData);
    if(iRet)
	{	
		Pax_Log(LOG_DEBUG,"Save_MiniData_ForMain,iRet=%d,fun:%s_line:%d",
							iRet,__FUNCTION__,__LINE__);
		return iRet;
	}
	return 0;
}

int ProcessStartUp(void)
{
	int iRet;
	
	iRet = menu_exec(stat_main_menu);
	if(iRet)
	{	
		Pax_Log(LOG_DEBUG,"menu_exec,iRet=%d,fun:%s_line:%d",
							iRet,__FUNCTION__,__LINE__);
		return iRet;
	}
	iRet = GetDataFromNodeStruc();
	if(iRet)
	{	
		Pax_Log(LOG_DEBUG,"GetDataFromNodeStruc,iRet=%d,fun:%s_line:%d",
							iRet,__FUNCTION__,__LINE__);
		return iRet;
	}
	iRet = SaveProductDataMiniposData();
	if(iRet)
	{	
		Pax_Log(LOG_DEBUG,"SaveProductData_MiniposData,iRet=%d,fun:%s_line:%d",
							iRet,__FUNCTION__,__LINE__);
		return iRet;
	}
	return 0;
}

int ProcessPaymentResult(PAYMENT_RESULT paymentResult)
{
	int iRet;
	int i = 0;
	int j = 0;
	
    memset(&glOrderAllProduct,0,sizeof(struct _orderAllProduct));
	iRet = ReadFile(ORDER_AllPRODUCT_FILE,&glOrderAllProduct,sizeof(struct _orderAllProduct));
	if(iRet)
	{
		return iRet;
	}
    switch(paymentResult)
    {
    	case MAIN_PAYRESULT_APPROVED:
			Pax_Log(LOG_INFO,"start to handleReversal 	fun:%s,line:%d",__FUNCTION__,__LINE__);
			iRet = HandleReversal();
			if(iRet)
			{
				Display_Prompt("FAIL", "Upload Order Product Fail" ,MSGTYPE_FAILURE, 0);
				HidePromptWin();
				Destroy_Display();
				break;
			}
			Pax_Log(LOG_INFO, "%s - %d HandleReversal", __FUNCTION__, __LINE__);
			iRet = Request_Process(CMD_UPLOAD_DATA,NORMAL);
			if(iRet)
			{
				Pax_Log(LOG_INFO, "Request_Process iRet=%d %s - %d ",iRet, __FUNCTION__, __LINE__);
				Display_Prompt("FAIL", "Upload Order Product Fail" ,MSGTYPE_FAILURE, 0);
				HidePromptWin();
				Destroy_Display();
				break;
				
			}
			iRet = PrintReceipt(glOrderAllProduct.orderLine,glOrderAllProduct.orderQuantity);
			if(iRet)
			{	
				Pax_Log(LOG_DEBUG,"PrintReceipt,iRet=%d,fun:%s_line:%d",
									iRet,__FUNCTION__,__LINE__);
				return iRet;
			}
			break;
		case MAIN_PAYRESULT_CANCELLED:
		case MAIN_PAYRESULT_DENIED:
			for(i = 0;i < glOrderAllProduct.orderQuantity;i++)
			{
				for(j = 0;j < product_list_node_count();j++)
				{
					void *node = product_list_node(j);
					if(glOrderAllProduct.orderLine[i].product_id == product_node_get_id(node))
					{
						product_node_set_price(node,(int)glOrderAllProduct.orderLine[i].price);
						product_node_set_count(node,glOrderAllProduct.orderLine[i].quantity);
						prouduct_node_set_vat(node,glOrderAllProduct.orderLine[i].vat);
					}
				}
			}
			iRet = menu_exec(stat_settle_menu);
			if(iRet)
			{	
				Pax_Log(LOG_DEBUG,"menu_exec,iRet=%d,fun:%s_line:%d",
							iRet,__FUNCTION__,__LINE__);
				return iRet;
			}
			iRet = GetDataFromNodeStruc();
			if(iRet)
			{	
				Pax_Log(LOG_DEBUG,"GetDataFromNodeStruc,iRet=%d,fun:%s_line:%d",
									iRet,__FUNCTION__,__LINE__);
				break;
			}
			iRet = SaveProductDataMiniposData();
			if(iRet)
			{	
				Pax_Log(LOG_DEBUG,"SaveProductData_MiniposData,iRet=%d,fun:%s_line:%d",
									iRet,__FUNCTION__,__LINE__);
				break;
			}
				break;
		default:
			break;
	}
	return iRet;
}

