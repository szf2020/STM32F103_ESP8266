#include "bsp_esp8266.h"
#include "bsp_usart1.h"
#include "bsp_usart3.h"
#include "bsp_systick.h"
#include <string.h>


void esp8266_init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  usart3_init(ESP8266_USART_BOUND);
  
  RCC_APB2PeriphClockCmd(ESP8266_CH_PD_CLK | ESP8266_RST_CLK, ENABLE);
  
  GPIO_InitStructure.GPIO_Pin = ESP8266_CH_PD_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(ESP8266_CH_PD_PORT, &GPIO_InitStructure);
  
  GPIO_InitStructure.GPIO_Pin = ESP8266_RST_PIN;
  GPIO_Init(ESP8266_RST_PORT, &GPIO_InitStructure);
  
  GPIO_SetBits(ESP8266_RST_PORT, ESP8266_RST_PIN); //�͵�ƽ��λ���ߵ�ƽ����
  GPIO_SetBits(ESP8266_CH_PD_PORT, ESP8266_CH_PD_PIN); //CH_PD:�ߵ�ƽ�������͵�ƽ����ص�

}

bool esp8266_sendCmd(char *cmd, char *reply, uint16_t wait)
{
  int i;
  rxFram.length = 0; //�������֡�����½���
  usart3_printf("%s\r\n", cmd);
  if(reply == 0)
    return true;

  delay_ms(wait);
  
  rxFram.rxbuffer[rxFram.length] = '\0'; //Ϊ���յ���֡���������ַ����������
  printf("%s",rxFram.rxbuffer);  //�����ݷ��͵�������λ��
  return ((bool) strstr(rxFram.rxbuffer,reply));
}

void esp8266_reset(void)
{
  GPIO_ResetBits(ESP8266_RST_PORT,ESP8266_RST_PIN);
  delay_ms(500);
  GPIO_SetBits(ESP8266_CH_PD_PORT, ESP8266_CH_PD_PIN);
}

bool esp8266_modeChoose(ESP8266_ModeEnumDef mode)
{
  switch(mode)
  {
    case STA:
      return esp8266_sendCmd("AT+CWMODE=1", "OK", 1500);
    case AP:
      return esp8266_sendCmd("AT+CWMODE=2", "OK", 1500);
    case STA_AP:
      return esp8266_sendCmd("AT+CWMODE=3", "OK", 1500);
    default:
      return false;
  }
}

bool esp8266_joinWifi(char *pSSID, char *pPassword)
{
  char cmd[120];
  sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"", pSSID, pPassword);
  return esp8266_sendCmd(cmd,"OK", 5000);
}

bool esp8266_buildWifi(char * pSSID, char * pPwd, ESP8266_ApPwdModeEnumDef pwdMode)
{
  char cmd[120];
  sprintf(cmd, "AT+CWSAP=\"%s\",\"%s\",1,%d",pSSID, pPwd, pwdMode);
  return esp8266_sendCmd(cmd, "OK", 1000);
}

bool esp8266_multipleIdCmd(FunctionalState state)
{
  char cmd[20];
  sprintf(cmd, "AT+CIPMUX=%d",state);
  return esp8266_sendCmd(cmd, "OK", 500);
}

bool esp8266_startServer ( char * pPortNum )
{
	char cmd [120];
	sprintf (cmd, "AT+CIPSERVER=%d,%s", 1, pPortNum );
	return( esp8266_sendCmd(cmd, "OK", 500));
}

bool esp8266_setTimeout(char * timeout)
{
  char cmd[20];
  sprintf(cmd, "AT+CIPSTO=%s", timeout);
  return esp8266_sendCmd(cmd, "OK", 500);
}

//bool esp8266_getApIp()
bool esp8266_getApIp (char * pApIp)
{
	char uc;
	char * pCh;
	
	
  esp8266_sendCmd("AT+CIFSR", "OK",500);
	
	pCh = strstr ( rxFram.rxbuffer, "APIP,\"" );
	
	if(pCh != 0)
		pCh += 6;
	else
		return 0;
	
	for ( uc = 0; uc < 20; uc ++ )
	{
		pApIp [ uc ] = * ( pCh + uc);
		
		if ( pApIp [ uc ] == '\"' )
		{
			pApIp [ uc ] = '\0';
			break;
		}		
	}
	return 1;
}

void esp8266_AT_Test ( void )
{
	char count=0;
	
	GPIO_SetBits(ESP8266_RST_PORT,ESP8266_RST_PIN);//macESP8266_RST_HIGH_LEVEL();	
	delay_ms ( 1000 );
	while ( count < 10 )
	{
		if( esp8266_sendCmd ( "AT", "OK", 500 ) ) return;
		esp8266_reset();
		++ count;
	}
}

void esp8266_AP_test(void)
{
  char pApIp[20];
  
  printf("��ʼ����esp8266\n");
  //esp8266_reset();
  //esp8266_AT_Test();
  while(!esp8266_sendCmd ( "AT", "OK", 1000 ));
  printf("ģ����Ӧ�ɹ�\r\n");
  esp8266_modeChoose(AP);
  printf("APģʽ���óɹ�\r\n");
  while(!esp8266_buildWifi(ESP8266_BUILD_AP_SSID,ESP8266_BUILD_AP_PWD,ESP8266_BUILD_AP_ECN));
  printf("wifi�����ɹ�\r\n");
  esp8266_multipleIdCmd(ENABLE);
  printf("����������ģʽ�ɹ�\r\n");
  esp8266_startServer(ESP8266_TCPSERVER_PORT);
  printf("���ñ��ض˿�\r\n");
  esp8266_setTimeout(ESP8266_TCPSERVER_TIMEOUT);
  esp8266_getApIp(pApIp);
  printf("����IP:%s\r\n",pApIp);
  
  printf("******�������******\n");
  
  rxFram.length = 0;
  rxFram.finishFlag = RESET;
  
  while(1)
  {
    if(rxFram.finishFlag == SET)
    {
      USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
      rxFram.rxbuffer[rxFram.length] = '\0';  //Ϊ֡�������ӽ������
      
      printf("%s",rxFram.rxbuffer);
      
      /*�������յ�������*/
      
      rxFram.length = 0;
      rxFram.finishFlag = RESET;
      USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); //�������ڽ����ж�
    }
  }
  
}

void esp8266_sta_tcpServer_test(void)
{
  char pApIp[20];
  
  printf("\r\n����ESP8266\r\n");
  
  while(!esp8266_sendCmd ( "AT", "OK", 1000 ));
  esp8266_modeChoose(STA);
  while(!esp8266_joinWifi(ESP8266_AP_SSID,ESP8266_AP_PWD));
  esp8266_multipleIdCmd(ENABLE);
  esp8266_startServer("8086");
  esp8266_setTimeout("500");
  esp8266_getApIp(pApIp);
  printf("\r\n�������\r\n");
  while(1)
  {
    if(rxFram.finishFlag == SET)
    {
      USART_ITConfig(USART3, USART_IT_RXNE, DISABLE);
      rxFram.rxbuffer[rxFram.length] = '\0';  //Ϊ֡�������ӽ������
      
      printf("%s",rxFram.rxbuffer);
      
      /*�������յ�������*/
      
      rxFram.length = 0;
      rxFram.finishFlag = RESET;
      USART_ITConfig(USART3, USART_IT_RXNE, ENABLE); //�������ڽ����ж�
    }
  }
}
