
// recordxDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "recordx.h"
#include "recordxDlg.h"
#include "afxdialogex.h"
#include <iostream>
#include <fstream>
#include <string>

#include "stdlib.h"
#include "stdio.h"
#include <windows.h>
#include <conio.h>
#include <errno.h>

#include "qisr.h"
#include "qtts.h"
#include "msp_cmn.h"
#include "msp_errors.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef _WIN64
#pragma comment(lib,"msc_x64.lib")//x64
#else
#pragma comment(lib,"msc.lib")//x86
#endif

#pragma comment(lib,"winmm.lib")
// CrecordxDlg 对话框
using namespace std;

CEdit m_editMultiLine;
char buf[1024 * 1024 * 4]; 

typedef int SR_DWORD;
typedef short int SR_WORD ;

#define MAXLINE 100000
#define BUFLEN  2048

//音频头部格式
struct wave_pcm_hdr
{
	char            riff[4];                        // = "RIFF"
	SR_DWORD        size_8;                         // = FileSize - 8
	char            wave[4];                        // = "WAVE"
	char            fmt[4];                         // = "fmt "
	SR_DWORD        dwFmtSize;                      // = 下一个结构体的大小 : 16

	SR_WORD         format_tag;              // = PCM : 1
	SR_WORD         channels;                       // = 通道数 : 1
	SR_DWORD        samples_per_sec;        // = 采样率 : 8000 | 6000 | 11025 | 16000
	SR_DWORD        avg_bytes_per_sec;      // = 每秒字节数 : dwSamplesPerSec * wBitsPerSample / 8
	SR_WORD         block_align;            // = 每采样点字节数 : wBitsPerSample / 8
	SR_WORD         bits_per_sample;         // = 量化比特数: 8 | 16

	char            data[4];                        // = "data";
	SR_DWORD        data_size;                // = 纯数据长度 : FileSize - 44 
} ;

//默认音频头部数据
struct wave_pcm_hdr default_pcmwavhdr = 
{
	{ 'R', 'I', 'F', 'F' },
	0,
	{'W', 'A', 'V', 'E'},
	{'f', 'm', 't', ' '},
	16,
	1,
	1,
	16000,
	32000,
	2,
	16,
	{'d', 'a', 't', 'a'},
	0  
};


CrecordxDlg::CrecordxDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CrecordxDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CrecordxDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_editMultiLine);
}

BEGIN_MESSAGE_MAP(CrecordxDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1,OnBnClickedButton1)
	ON_BN_CLICKED(IDC_BUTTON2,OnBnClickedButton2)
	ON_BN_CLICKED(IDC_BUTTON3,OnBnClickedButton3)

	ON_MESSAGE(MM_WIM_OPEN,OnMM_WIM_OPEN)
	ON_MESSAGE(MM_WIM_DATA,OnMM_WIM_DATA)
	ON_MESSAGE(MM_WIM_CLOSE,OnMM_WIM_CLOSE)
	ON_MESSAGE(MM_WOM_OPEN,OnMM_WON_OPEN)
	ON_MESSAGE(MM_WOM_DONE,OnMM_WOM_DONE)
	ON_MESSAGE(MM_WOM_CLOSE,OnMM_WOM_CLOSE)
	

	ON_BN_CLICKED(IDC_BUTTON4, &CrecordxDlg::OnBnClickedButton4)
	ON_BN_CLICKED(IDC_BUTTON5, &CrecordxDlg::OnBnClickedButton5)
	ON_EN_CHANGE(IDC_EDIT1, &CrecordxDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_BUTTON6, &CrecordxDlg::OnBnClickedButton6)
	ON_BN_CLICKED(IDC_BUTTON7, &CrecordxDlg::OnBnClickedButton7)
END_MESSAGE_MAP()


// CrecordxDlg 消息处理程序

BOOL CrecordxDlg::OnInitDialog()
{

	CDialogEx::OnInitDialog();


	/*ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);   
    ASSERT(IDM_ABOUTBOX < 0xF000);   
  
    CMenu* pSysMenu = GetSystemMenu(FALSE);   
    if (pSysMenu != NULL)   
    {   
        BOOL bNameValid;   
        CString strAboutMenu;   
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);   
        ASSERT(bNameValid);   
        if (!strAboutMenu.IsEmpty())   
        {   
            pSysMenu->AppendMenu(MF_SEPARATOR);   
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);   
        }   
    }   */
  
	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	//录音机状态初始化
	//m_nRecordState = RECORD_STATE_INIT;
	m_dwDataLength = 0;

	//open waveform audo for input
	//为声音输入设置格式
	m_waveform.wFormatTag=WAVE_FORMAT_PCM;
	m_waveform.nChannels=1;     //单双通道，通常设置为1/2
	m_waveform.nSamplesPerSec=16000;//此处设置采样率,可以有多种参数设置
	m_waveform.nAvgBytesPerSec=16000*2;     //此处等于采样率乘以BlockAlign
	m_waveform.nBlockAlign=2;//此处设置要看BitsPerSample是如何设置的，如果是8，就为1，如果是16，就为2
	m_waveform.wBitsPerSample=16;//采样精度，通常设置为8/16
	m_waveform.cbSize=0;    
    
	m_pSaveBuffer = NULL;

    if(waveInOpen(&m_hWaveIn,WAVE_MAPPER,&m_waveform,
            (DWORD)m_hWnd,NULL,CALLBACK_WINDOW)){
        MessageBeep(MB_ICONEXCLAMATION);
        MessageBox(_T("录制声音失败!"),
                    _T("错误"),MB_ICONEXCLAMATION|MB_OK);
        return TRUE;
    }

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CrecordxDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CrecordxDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CrecordxDlg::OnBnClickedButton1()
{
	// TODO: 在此添加控件通知处理程序代码
	//打开声音设备
	CString szTitle3;
    GetDlgItemText(IDC_BUTTON4 ,szTitle3);
    if (szTitle3 == "已保存")
    {    
        SetDlgItemText(IDC_BUTTON4 ,"保存录音");  //
        // 添加你的"打开"按钮处理代码
		//SetDlgItemText(IDC_BUTTON4 ,"已保存"); 
	 // waveInClose (m_hWaveIn) ;//停止录音，关闭输入设备  
    }	

	CString szTitle1;
    GetDlgItemText(IDC_BUTTON1 ,szTitle1);

    //设置缓冲区
    m_WAVEHDR1.lpData = (LPSTR)m_cbBuffer1;
    m_WAVEHDR1.dwBufferLength = INP_BUFFER_SIZE;
    m_WAVEHDR1.dwBytesRecorded = 0;
    m_WAVEHDR1.dwUser=0;
    m_WAVEHDR1.dwFlags=0;
    m_WAVEHDR1.dwLoops=1;
    m_WAVEHDR1.lpNext=NULL;
    m_WAVEHDR1.reserved=0;

    m_WAVEHDR2.lpData=(LPSTR)m_cbBuffer2;
    m_WAVEHDR2.dwBufferLength = INP_BUFFER_SIZE;
    m_WAVEHDR2.dwBytesRecorded = 0;
    m_WAVEHDR2.dwUser=0;
    m_WAVEHDR2.dwFlags=0;
    m_WAVEHDR2.dwLoops=1;
    m_WAVEHDR2.lpNext=NULL;
    m_WAVEHDR2.reserved=0;

    //设置双缓冲
    waveInPrepareHeader(m_hWaveIn,&m_WAVEHDR1,sizeof(WAVEHDR));
    waveInPrepareHeader(m_hWaveIn,&m_WAVEHDR2,sizeof(WAVEHDR));

    waveInAddBuffer(m_hWaveIn,&m_WAVEHDR1,sizeof(WAVEHDR));
    waveInAddBuffer(m_hWaveIn,&m_WAVEHDR2,sizeof(WAVEHDR));

    //先随便分配个内存
    if(m_pSaveBuffer != NULL){//2015.11.20
        free(m_pSaveBuffer);
        m_pSaveBuffer = NULL;
    }
    m_pSaveBuffer = (PBYTE)malloc(1);
    m_dwDataLength = 0;


    //开始录音
    // Begin sampling    
    waveInStart(m_hWaveIn);

	if (szTitle1 == "开始录音")
    {
        SetDlgItemText(IDC_BUTTON1 ,"正在录音...");  //
        // 添加你的"打开"按钮处理代码
    }
}

LRESULT CrecordxDlg::OnMM_WIM_CLOSE(UINT wParam, LONG lParam)
{
    //waveInUnprepareHeader(m_hWaveIn, &m_WAVEHDR1, sizeof (WAVEHDR)) ;
    //waveInUnprepareHeader(m_hWaveIn, &m_WAVEHDR2, sizeof (WAVEHDR)) ;
    
    //m_nRecordState = RECORD_STATE_STOP;
    //SetButtonState();

    return NULL;
}

LRESULT CrecordxDlg::OnMM_WIM_DATA(UINT wParam, LONG lParam)
{
    PWAVEHDR pWaveHdr = (PWAVEHDR)lParam;
    
    if(pWaveHdr->dwBytesRecorded > 0){
        m_pSaveBuffer = (PBYTE)realloc(m_pSaveBuffer,m_dwDataLength + pWaveHdr->dwBytesRecorded);
        if(m_pSaveBuffer == NULL){
            waveInClose (m_hWaveIn);
            MessageBeep (MB_ICONEXCLAMATION) ;
            //AfxMessageBox("erro memory");
            return NULL;
        }

        memcpy(m_pSaveBuffer+m_dwDataLength , pWaveHdr->lpData,pWaveHdr->dwBytesRecorded);

        m_dwDataLength += pWaveHdr->dwBytesRecorded;
    }

    //if(m_nRecordState == RECORD_STATE_STOPING){
        //waveInClose(m_hWaveIn);        
   // }

    waveInAddBuffer(m_hWaveIn, pWaveHdr, sizeof (WAVEHDR));
	return NULL;
}

LRESULT CrecordxDlg::OnMM_WIM_OPEN(UINT wParam, LONG lParam)
{
    //m_nRecordState = RECORD_STATE_RECING;
	//waveInClose (m_hWaveIn);
    //SetButtonState();
   return NULL;
}

void CrecordxDlg::OnBnClickedButton2()
{
	// TODO: 在此添加控件通知处理程序代码
	//m_nRecordState = RECORD_STATE_STOPING;
	CString rec_result1;
	m_editMultiLine.GetWindowText(rec_result1);
	ofstream myfile("文本记录.txt",ios::app); 
	myfile<<rec_result1;
	//myfile.close();
}


void CrecordxDlg::OnBnClickedButton3()
{
	// TODO: 在此添加控件通知处理程序代码   
	CString rec_result1;
	m_editMultiLine.GetWindowText(rec_result1);
	ShellExecute(NULL,"open","https://www.baidu.com/s?wd="+rec_result1,NULL,NULL,SW_SHOW);

}


LRESULT CrecordxDlg::OnMM_WON_OPEN(UINT wParam,LONG lParam)
{
    memset(&m_WAVEHDR1,0,sizeof(WAVEHDR));
    m_WAVEHDR1.lpData = (char *)m_pSaveBuffer;//指向要播放的内存
    m_WAVEHDR1.dwBufferLength = m_dwDataLength;//播放的长度
    m_WAVEHDR1.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
    m_WAVEHDR1.dwLoops = 1;

    waveOutPrepareHeader(m_hWaveOut,&m_WAVEHDR1,sizeof(WAVEHDR));

    waveOutWrite(m_hWaveOut,&m_WAVEHDR1,sizeof(WAVEHDR));

    m_nRecordState = RECORD_STATE_PLAYING;
    //SetButtonState();
    return NULL;
}

LRESULT CrecordxDlg::OnMM_WOM_DONE(UINT wParam,LONG lParam)
{
    PWAVEHDR pWaveHdr = (PWAVEHDR)lParam;
    waveOutUnprepareHeader (m_hWaveOut, pWaveHdr, sizeof (WAVEHDR)) ;
    waveOutClose (m_hWaveOut);
    return NULL;
}

LRESULT CrecordxDlg::OnMM_WOM_CLOSE(UINT wParam,LONG lParam)
{
    m_nRecordState = RECORD_STATE_STOP;
    //SetButtonState();
    return NULL;
}

void CrecordxDlg::OnBnClickedButton4()
{
	// TODO: 在此添加控件通知处理程序代码

	waveInClose (m_hWaveIn) ;//停止录音，关闭输入设备  

	CFile m_file;

	CFileException fileException;

	CString m_csFileName= "audio.wav";//保存路径


	m_file.Open(m_csFileName,CFile::modeCreate|CFile::modeReadWrite,&fileException);

	DWORD m_WaveHeaderSize = 38;

	DWORD m_WaveFormatSize = 18;

	m_file.SeekToBegin();

	m_file.Write("RIFF",4);

	unsigned int Sec=(sizeof m_pSaveBuffer + m_WaveHeaderSize);

	m_file.Write(&Sec,sizeof(Sec));

	m_file.Write("WAVE",4);

	m_file.Write("fmt ",4);

	m_file.Write(&m_WaveFormatSize,sizeof(m_WaveFormatSize));

	m_file.Write(&m_waveform.wFormatTag,sizeof(m_waveform.wFormatTag));

	m_file.Write(&m_waveform.nChannels,sizeof(m_waveform.nChannels));

	m_file.Write(&m_waveform.nSamplesPerSec,sizeof(m_waveform.nSamplesPerSec));

	m_file.Write(&m_waveform.nAvgBytesPerSec,sizeof(m_waveform.nAvgBytesPerSec));

	m_file.Write(&m_waveform.nBlockAlign,sizeof(m_waveform.nBlockAlign));

	m_file.Write(&m_waveform.wBitsPerSample,sizeof(m_waveform.wBitsPerSample));

	m_file.Write(&m_waveform.cbSize,sizeof(m_waveform.cbSize));

	m_file.Write("data",4);

	m_file.Write(&m_dwDataLength,sizeof(m_dwDataLength));

	m_file.Write(m_pSaveBuffer,m_dwDataLength);

	m_file.Seek(m_dwDataLength,CFile::begin);

	m_file.Close();

	CString szTitle2;
    GetDlgItemText(IDC_BUTTON4 ,szTitle2);
    if (szTitle2 == "保存录音")
    {
        SetDlgItemText(IDC_BUTTON1 ,"开始录音");  //
        // 添加你的"打开"按钮处理代码
		SetDlgItemText(IDC_BUTTON4 ,"已保存"); 
	 // waveInClose (m_hWaveIn) ;//停止录音，关闭输入设备  
    }	

	free(m_pSaveBuffer);
    m_pSaveBuffer = NULL;
}

void run_iat(const char* src_wav_filename ,  const char* param)
{
	char rec_result[1024] = {0};
	const char *sessionID = NULL;
	FILE *f_pcm = NULL;
	char *pPCM = NULL;
	int lastAudio = 0 ;
	//int audStat = MSP_AUDIO_SAMPLE_CONTINUE ;
	int audStat = 2;
	//int epStatus = MSP_EP_LOOKING_FOR_SPEECH;
	int epStatus = 0;
	//int recStatus = MSP_REC_STATUS_SUCCESS ;
	int recStatus = 0;
	long pcmCount = 0;
	long pcmSize = 0;
	int errCode = 10 ;
	
	sessionID = QISRSessionBegin(NULL, param, &errCode);//开始一路会话
	f_pcm = fopen(src_wav_filename, "rb");
	if (NULL != f_pcm) {
		fseek(f_pcm, 0, SEEK_END);
		pcmSize = ftell(f_pcm);
		fseek(f_pcm, 0, SEEK_SET);
		pPCM = (char *)malloc(pcmSize);
		fread((void *)pPCM, pcmSize, 1, f_pcm);
		fclose(f_pcm);
		f_pcm = NULL;
	}//读取音频文件
	while (1) {
		unsigned int len = 6400;
		int ret = 0;
		if (pcmSize < 12800) {
			len = pcmSize;
			lastAudio = 1;//音频长度小于12800
		}
		audStat = MSP_AUDIO_SAMPLE_CONTINUE;//有后继音频
		if (pcmCount == 0)
			audStat = MSP_AUDIO_SAMPLE_FIRST;
		//if (len<=0)
		//{
		//	break;
		//}
//		printf("csid=%s,count=%d,aus=%d,",sessionID,pcmCount/len,audStat);
		ret = QISRAudioWrite(sessionID, (const void *)&pPCM[pcmCount], len, audStat, &epStatus, &recStatus);//写音频
//		printf("eps=%d,rss=%d,ret=%d\n",epStatus,recStatus,errCode);
		if (ret != 0)
		break;
		pcmCount += (long)len;
		pcmSize -= (long)len;
		if (recStatus == 0) {
			const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &errCode);//服务端已经有识别结果，可以获取
			if (NULL != rslt)
				strcat(rec_result,rslt);
		}
		if (epStatus == 3)
			break;
		_sleep(150);//模拟人说话时间间隙
	}
	QISRAudioWrite(sessionID, (const void *)NULL, 0, MSP_AUDIO_SAMPLE_LAST, &epStatus, &recStatus);
	free(pPCM);
	pPCM = NULL;
	while (recStatus != 5 && 0 == errCode) {
		const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &errCode);//获取结果
		if (NULL != rslt)
		{
			strcat(rec_result,rslt);
		}
		_sleep(150);
	}
	QISRSessionEnd(sessionID, NULL);
	//printf("=============================================================\n");
	//printf("The result is: %s\n",rec_result);
	//printf("=============================================================\n");
	//CString str1(&rec_result[0],&rec_result[strlen(rec_result)]);
	//CString str1;
	//str1.Format();
	//AfxMessageBox(rec_result);
	m_editMultiLine.SetWindowText(_T(rec_result));

	//SetDlgItemText(IDC_EDIT1,rec_result);

}

void CrecordxDlg::OnBnClickedButton5()
{
	// TODO: 在此添加控件通知处理程序代码
	///APPID请勿随意改动
	const char* login_configs = "appid = 55ed5156, work_dir =   .  ";//登录参数
	const char* param1 = "sub=iat,auf=audio/L16;rate=16000,aue=raw,ent=sms16k,rst=plain,rse=gb2312";//可参考可设置参数列表
	//const char* param2 = "sub=iat,auf=audio/L16;rate=16000,aue=speex-wb,ent=sms16k,rst=json,rse=utf8";//转写为json格式，编码只能为utf8
	int ret = 0;
	char key = 0;

	//用户登录
	ret = MSPLogin(NULL, NULL, login_configs);//第一个参数为用户名，第二个参数为密码，第三个参数是登录参数，用户名和密码需要在http://open.voicecloud.cn注册并获取appid
	if ( ret != 0 )
	{
		AfxMessageBox("MSPLogin failed , Error code %d.\n",ret);
		goto exit ;
	}
	//开始一路转写会话
	run_iat("audio.wav" ,  param1);                                     //iflytek09对应的音频内容“沉舟侧畔千帆过，病树前头万木春。”
//	run_iat("wav/iflytek01.wav" ,  param2);                                     //iflytek01对应的音频内容“科大讯飞”

	exit:
    MSPLogout();//退出登录
	key = _getch();
}


void CrecordxDlg::OnEnChangeEdit1()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
}


int text_to_speech(const char* src_text ,const char* des_path ,const char* params)
{
	struct wave_pcm_hdr pcmwavhdr = default_pcmwavhdr;
	const char* sess_id = NULL;
	int ret = -1;
	unsigned int text_len = 0;
	char* audio_data = NULL;
	unsigned int audio_len = 0;
	int synth_status = MSP_TTS_FLAG_STILL_HAVE_DATA;
	FILE* fp = NULL;

	//AfxMessageBox("begin to synth...\n");
	if (NULL == src_text || NULL == des_path)
	{
		AfxMessageBox("params is null!\n");
		return ret;
	}
	text_len = (unsigned int)strlen(src_text);
	fp = fopen(des_path,"wb");
	if (NULL == fp)
	{
		printf("open file %s error\n",des_path);
		return ret;
	}
	sess_id = QTTSSessionBegin(params, &ret);
	if ( ret != MSP_SUCCESS )
	{
		AfxMessageBox("QTTSSessionBegin: qtts begin session failed Error code %d.\n",ret);
		return ret;
	}

	ret = QTTSTextPut(sess_id, src_text, text_len, NULL );
	if ( ret != MSP_SUCCESS )
	{
		AfxMessageBox("QTTSTextPut: qtts put text failed Error code %d.\n",ret);
		QTTSSessionEnd(sess_id, "TextPutError");
		return ret;
	}
	fwrite(&pcmwavhdr, sizeof(pcmwavhdr) ,1, fp);
	while (1) 
	{
		const void *data = QTTSAudioGet(sess_id, &audio_len, &synth_status, &ret);
		if (NULL != data)
		{
			fwrite(data, audio_len, 1, fp);
		    pcmwavhdr.data_size += audio_len;//修正pcm数据的大小
		}
		if (synth_status == MSP_TTS_FLAG_DATA_END || ret != 0) 
			break;
	}//合成状态synth_status取值可参考开发文档

	//修正pcm文件头数据的大小
	pcmwavhdr.size_8 += pcmwavhdr.data_size + 36;

	//将修正过的数据写回文件头部
	fseek(fp, 4, 0);
	fwrite(&pcmwavhdr.size_8,sizeof(pcmwavhdr.size_8), 1, fp);
	fseek(fp, 40, 0);
	fwrite(&pcmwavhdr.data_size,sizeof(pcmwavhdr.data_size), 1, fp);
	fclose(fp);

	ret = QTTSSessionEnd(sess_id, NULL);
	if ( ret != MSP_SUCCESS )
	{
		AfxMessageBox("QTTSSessionEnd: qtts end failed Error code %d.\n",ret);
	}
	return ret;
}

void CrecordxDlg::OnBnClickedButton6()
{
	// TODO: 在此添加控件通知处理程序代码
	const char* login_configs = " appid = 55ed5156,work_dir =   .  ";//登录参数

	CString text;
	m_editMultiLine.GetWindowText(text);
	//const char* text  = "却没有好坏之分。";//待合成的文本
	const char*  filename = "text_to_speech_test.pcm"; //合成的语音文件
	const char* param = "vcn=xiaoyan, spd = 70, vol = 50";//参数可参考可设置参数列表
	int ret = 0;
	char key = 0;

	//用户登录
	ret = MSPLogin(NULL, NULL, login_configs);//第一个参数为用户名，第二个参数为密码，第三个参数是登录参数，用户名和密码需要在http://open.voicecloud.cn注册并获取appid
	if ( ret != MSP_SUCCESS )
	{
		AfxMessageBox("MSPLogin failed , Error code %d.\n",ret);
		goto exit ;//登录失败，退出登录
	}
	//音频合成
	ret = text_to_speech(text,filename,param);//音频合成
	if ( ret != MSP_SUCCESS )
	{
		AfxMessageBox("text_to_speech: failed , Error code %d.\n",ret);
		goto exit ;//合成失败，退出登录
	}
	exit:
    MSPLogout();//退出登录
	

	FILE*           thbgm;//文件  
    int             cnt;  
    HWAVEOUT        hwo;  
    WAVEHDR         wh;  
    WAVEFORMATEX    wfx;  
    HANDLE          wait;  
  
    wfx.wFormatTag = WAVE_FORMAT_PCM;//设置波形声音的格式  
    wfx.nChannels = 1;//设置音频文件的通道数量  
    wfx.nSamplesPerSec = 16000;//设置每个声道播放和记录时的样本频率  
    wfx.nAvgBytesPerSec = 32000;//设置请求的平均数据传输率,单位byte/s。这个值对于创建缓冲大小是很有用的  
    wfx.nBlockAlign = 2;//以字节为单位设置块对齐  
    wfx.wBitsPerSample = 16;  
    wfx.cbSize = 0;//额外信息的大小  
    wait = CreateEvent(NULL, 0, 0, NULL);  
    waveOutOpen(&hwo, WAVE_MAPPER, &wfx, (DWORD_PTR)wait, 0L, CALLBACK_EVENT);//打开一个给定的波形音频输出装置来进行回放  
    fopen_s(&thbgm, "text_to_speech_test.pcm", "rb"); 
	int mcount=0;
	fread(buf, sizeof(struct wave_pcm_hdr), 1, thbgm);
	while(fread(buf, sizeof(char), 1024, thbgm)==1024)
	{
		mcount++;
	}
	fseek(thbgm,0,SEEK_SET);
    cnt = fread(buf, sizeof(char), 1024 * (mcount) , thbgm);//读取文件4M的数据到内存来进行播放，通过这个部分的修改，增加线程可变成网络音频数据的实时传输。当然如果希望播放完整的音频文件，也是要在这里稍微改一改  
    int dolenght = 0;  
    int playsize = 1024;  
    while (cnt) {//这一部分需要特别注意的是在循环回来之后不能花太长的时间去做读取数据之类的工作，不然在每个循环的间隙会有“哒哒”的噪音  
        wh.lpData = buf + dolenght;  
        wh.dwBufferLength = playsize;  
        wh.dwFlags = 0L;  
        wh.dwLoops = 1L;  
        waveOutPrepareHeader(hwo, &wh, sizeof(WAVEHDR));//准备一个波形数据块用于播放  
        waveOutWrite(hwo, &wh, sizeof(WAVEHDR));//在音频媒体中播放第二个函数wh指定的数据  
        WaitForSingleObject(wait, INFINITE);//用来检测hHandle事件的信号状态，在某一线程中调用该函数时，线程暂时挂起，如果在挂起的INFINITE毫秒内，线程所等待的对象变为有信号状态，则该函数立即返回  
        dolenght = dolenght + playsize;  
        cnt = cnt - playsize;  
    }  
    waveOutClose(hwo);  
    fclose(thbgm);  
	key = _getch();//退出
}


void CrecordxDlg::OnBnClickedButton7()
{
	// TODO: 在此添加控件通知处理程序代码

	//CString text="中国人的饮食更加注重味道，不同的地域对菜品的味道也有不同的偏好，例如重庆人喜欢";
	
	FILE *file;
	char buf[BUFLEN];
	int len=0,i=0;
	char *array[MAXLINE];

	file=fopen("文本记录.txt","r");//打开TXST.TxT文件
	
	while(fgets(buf,BUFLEN,file))//读取TXT中字符
	{
	   len=strlen(buf);
	   array[i]=(char*)malloc(len+1);
	   if(!array[i])break;
	   strcpy(array[i++],buf);
	}

	fclose(file);
	i--;
	while(i>=0&&array[i])
	{
	  // printf("%s\n",array[i]);//打印test文档的字符
	   m_editMultiLine.SetWindowText(_T(array[i]));
	   free(array[i--]);
	}
	
	//const char* text  = "却没有好坏之分。";//待合成的文本
}
