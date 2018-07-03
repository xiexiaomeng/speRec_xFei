
// recordxDlg.cpp : ʵ���ļ�
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
// CrecordxDlg �Ի���
using namespace std;

CEdit m_editMultiLine;
char buf[1024 * 1024 * 4]; 

typedef int SR_DWORD;
typedef short int SR_WORD ;

#define MAXLINE 100000
#define BUFLEN  2048

//��Ƶͷ����ʽ
struct wave_pcm_hdr
{
	char            riff[4];                        // = "RIFF"
	SR_DWORD        size_8;                         // = FileSize - 8
	char            wave[4];                        // = "WAVE"
	char            fmt[4];                         // = "fmt "
	SR_DWORD        dwFmtSize;                      // = ��һ���ṹ��Ĵ�С : 16

	SR_WORD         format_tag;              // = PCM : 1
	SR_WORD         channels;                       // = ͨ���� : 1
	SR_DWORD        samples_per_sec;        // = ������ : 8000 | 6000 | 11025 | 16000
	SR_DWORD        avg_bytes_per_sec;      // = ÿ���ֽ��� : dwSamplesPerSec * wBitsPerSample / 8
	SR_WORD         block_align;            // = ÿ�������ֽ��� : wBitsPerSample / 8
	SR_WORD         bits_per_sample;         // = ����������: 8 | 16

	char            data[4];                        // = "data";
	SR_DWORD        data_size;                // = �����ݳ��� : FileSize - 44 
} ;

//Ĭ����Ƶͷ������
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


// CrecordxDlg ��Ϣ�������

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
  
	// ���ô˶Ի����ͼ�ꡣ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������

	//¼����״̬��ʼ��
	//m_nRecordState = RECORD_STATE_INIT;
	m_dwDataLength = 0;

	//open waveform audo for input
	//Ϊ�����������ø�ʽ
	m_waveform.wFormatTag=WAVE_FORMAT_PCM;
	m_waveform.nChannels=1;     //��˫ͨ����ͨ������Ϊ1/2
	m_waveform.nSamplesPerSec=16000;//�˴����ò�����,�����ж��ֲ�������
	m_waveform.nAvgBytesPerSec=16000*2;     //�˴����ڲ����ʳ���BlockAlign
	m_waveform.nBlockAlign=2;//�˴�����Ҫ��BitsPerSample��������õģ������8����Ϊ1�������16����Ϊ2
	m_waveform.wBitsPerSample=16;//�������ȣ�ͨ������Ϊ8/16
	m_waveform.cbSize=0;    
    
	m_pSaveBuffer = NULL;

    if(waveInOpen(&m_hWaveIn,WAVE_MAPPER,&m_waveform,
            (DWORD)m_hWnd,NULL,CALLBACK_WINDOW)){
        MessageBeep(MB_ICONEXCLAMATION);
        MessageBox(_T("¼������ʧ��!"),
                    _T("����"),MB_ICONEXCLAMATION|MB_OK);
        return TRUE;
    }

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CrecordxDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CrecordxDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CrecordxDlg::OnBnClickedButton1()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//�������豸
	CString szTitle3;
    GetDlgItemText(IDC_BUTTON4 ,szTitle3);
    if (szTitle3 == "�ѱ���")
    {    
        SetDlgItemText(IDC_BUTTON4 ,"����¼��");  //
        // ������"��"��ť�������
		//SetDlgItemText(IDC_BUTTON4 ,"�ѱ���"); 
	 // waveInClose (m_hWaveIn) ;//ֹͣ¼�����ر������豸  
    }	

	CString szTitle1;
    GetDlgItemText(IDC_BUTTON1 ,szTitle1);

    //���û�����
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

    //����˫����
    waveInPrepareHeader(m_hWaveIn,&m_WAVEHDR1,sizeof(WAVEHDR));
    waveInPrepareHeader(m_hWaveIn,&m_WAVEHDR2,sizeof(WAVEHDR));

    waveInAddBuffer(m_hWaveIn,&m_WAVEHDR1,sizeof(WAVEHDR));
    waveInAddBuffer(m_hWaveIn,&m_WAVEHDR2,sizeof(WAVEHDR));

    //����������ڴ�
    if(m_pSaveBuffer != NULL){//2015.11.20
        free(m_pSaveBuffer);
        m_pSaveBuffer = NULL;
    }
    m_pSaveBuffer = (PBYTE)malloc(1);
    m_dwDataLength = 0;


    //��ʼ¼��
    // Begin sampling    
    waveInStart(m_hWaveIn);

	if (szTitle1 == "��ʼ¼��")
    {
        SetDlgItemText(IDC_BUTTON1 ,"����¼��...");  //
        // ������"��"��ť�������
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//m_nRecordState = RECORD_STATE_STOPING;
	CString rec_result1;
	m_editMultiLine.GetWindowText(rec_result1);
	ofstream myfile("�ı���¼.txt",ios::app); 
	myfile<<rec_result1;
	//myfile.close();
}


void CrecordxDlg::OnBnClickedButton3()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������   
	CString rec_result1;
	m_editMultiLine.GetWindowText(rec_result1);
	ShellExecute(NULL,"open","https://www.baidu.com/s?wd="+rec_result1,NULL,NULL,SW_SHOW);

}


LRESULT CrecordxDlg::OnMM_WON_OPEN(UINT wParam,LONG lParam)
{
    memset(&m_WAVEHDR1,0,sizeof(WAVEHDR));
    m_WAVEHDR1.lpData = (char *)m_pSaveBuffer;//ָ��Ҫ���ŵ��ڴ�
    m_WAVEHDR1.dwBufferLength = m_dwDataLength;//���ŵĳ���
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	waveInClose (m_hWaveIn) ;//ֹͣ¼�����ر������豸  

	CFile m_file;

	CFileException fileException;

	CString m_csFileName= "audio.wav";//����·��


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
    if (szTitle2 == "����¼��")
    {
        SetDlgItemText(IDC_BUTTON1 ,"��ʼ¼��");  //
        // ������"��"��ť�������
		SetDlgItemText(IDC_BUTTON4 ,"�ѱ���"); 
	 // waveInClose (m_hWaveIn) ;//ֹͣ¼�����ر������豸  
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
	
	sessionID = QISRSessionBegin(NULL, param, &errCode);//��ʼһ·�Ự
	f_pcm = fopen(src_wav_filename, "rb");
	if (NULL != f_pcm) {
		fseek(f_pcm, 0, SEEK_END);
		pcmSize = ftell(f_pcm);
		fseek(f_pcm, 0, SEEK_SET);
		pPCM = (char *)malloc(pcmSize);
		fread((void *)pPCM, pcmSize, 1, f_pcm);
		fclose(f_pcm);
		f_pcm = NULL;
	}//��ȡ��Ƶ�ļ�
	while (1) {
		unsigned int len = 6400;
		int ret = 0;
		if (pcmSize < 12800) {
			len = pcmSize;
			lastAudio = 1;//��Ƶ����С��12800
		}
		audStat = MSP_AUDIO_SAMPLE_CONTINUE;//�к����Ƶ
		if (pcmCount == 0)
			audStat = MSP_AUDIO_SAMPLE_FIRST;
		//if (len<=0)
		//{
		//	break;
		//}
//		printf("csid=%s,count=%d,aus=%d,",sessionID,pcmCount/len,audStat);
		ret = QISRAudioWrite(sessionID, (const void *)&pPCM[pcmCount], len, audStat, &epStatus, &recStatus);//д��Ƶ
//		printf("eps=%d,rss=%d,ret=%d\n",epStatus,recStatus,errCode);
		if (ret != 0)
		break;
		pcmCount += (long)len;
		pcmSize -= (long)len;
		if (recStatus == 0) {
			const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &errCode);//������Ѿ���ʶ���������Ի�ȡ
			if (NULL != rslt)
				strcat(rec_result,rslt);
		}
		if (epStatus == 3)
			break;
		_sleep(150);//ģ����˵��ʱ���϶
	}
	QISRAudioWrite(sessionID, (const void *)NULL, 0, MSP_AUDIO_SAMPLE_LAST, &epStatus, &recStatus);
	free(pPCM);
	pPCM = NULL;
	while (recStatus != 5 && 0 == errCode) {
		const char *rslt = QISRGetResult(sessionID, &recStatus, 0, &errCode);//��ȡ���
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	///APPID��������Ķ�
	const char* login_configs = "appid = 55ed5156, work_dir =   .  ";//��¼����
	const char* param1 = "sub=iat,auf=audio/L16;rate=16000,aue=raw,ent=sms16k,rst=plain,rse=gb2312";//�ɲο������ò����б�
	//const char* param2 = "sub=iat,auf=audio/L16;rate=16000,aue=speex-wb,ent=sms16k,rst=json,rse=utf8";//תдΪjson��ʽ������ֻ��Ϊutf8
	int ret = 0;
	char key = 0;

	//�û���¼
	ret = MSPLogin(NULL, NULL, login_configs);//��һ������Ϊ�û������ڶ�������Ϊ���룬�����������ǵ�¼�������û�����������Ҫ��http://open.voicecloud.cnע�Ტ��ȡappid
	if ( ret != 0 )
	{
		AfxMessageBox("MSPLogin failed , Error code %d.\n",ret);
		goto exit ;
	}
	//��ʼһ·תд�Ự
	run_iat("audio.wav" ,  param1);                                     //iflytek09��Ӧ����Ƶ���ݡ����۲���ǧ����������ǰͷ��ľ������
//	run_iat("wav/iflytek01.wav" ,  param2);                                     //iflytek01��Ӧ����Ƶ���ݡ��ƴ�Ѷ�ɡ�

	exit:
    MSPLogout();//�˳���¼
	key = _getch();
}


void CrecordxDlg::OnEnChangeEdit1()
{
	// TODO:  ����ÿؼ��� RICHEDIT �ؼ���������
	// ���ʹ�֪ͨ��������д CDialogEx::OnInitDialog()
	// ���������� CRichEditCtrl().SetEventMask()��
	// ͬʱ�� ENM_CHANGE ��־�������㵽�����С�

	// TODO:  �ڴ���ӿؼ�֪ͨ����������
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
		    pcmwavhdr.data_size += audio_len;//����pcm���ݵĴ�С
		}
		if (synth_status == MSP_TTS_FLAG_DATA_END || ret != 0) 
			break;
	}//�ϳ�״̬synth_statusȡֵ�ɲο������ĵ�

	//����pcm�ļ�ͷ���ݵĴ�С
	pcmwavhdr.size_8 += pcmwavhdr.data_size + 36;

	//��������������д���ļ�ͷ��
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	const char* login_configs = " appid = 55ed5156,work_dir =   .  ";//��¼����

	CString text;
	m_editMultiLine.GetWindowText(text);
	//const char* text  = "ȴû�кû�֮�֡�";//���ϳɵ��ı�
	const char*  filename = "text_to_speech_test.pcm"; //�ϳɵ������ļ�
	const char* param = "vcn=xiaoyan, spd = 70, vol = 50";//�����ɲο������ò����б�
	int ret = 0;
	char key = 0;

	//�û���¼
	ret = MSPLogin(NULL, NULL, login_configs);//��һ������Ϊ�û������ڶ�������Ϊ���룬�����������ǵ�¼�������û�����������Ҫ��http://open.voicecloud.cnע�Ტ��ȡappid
	if ( ret != MSP_SUCCESS )
	{
		AfxMessageBox("MSPLogin failed , Error code %d.\n",ret);
		goto exit ;//��¼ʧ�ܣ��˳���¼
	}
	//��Ƶ�ϳ�
	ret = text_to_speech(text,filename,param);//��Ƶ�ϳ�
	if ( ret != MSP_SUCCESS )
	{
		AfxMessageBox("text_to_speech: failed , Error code %d.\n",ret);
		goto exit ;//�ϳ�ʧ�ܣ��˳���¼
	}
	exit:
    MSPLogout();//�˳���¼
	

	FILE*           thbgm;//�ļ�  
    int             cnt;  
    HWAVEOUT        hwo;  
    WAVEHDR         wh;  
    WAVEFORMATEX    wfx;  
    HANDLE          wait;  
  
    wfx.wFormatTag = WAVE_FORMAT_PCM;//���ò��������ĸ�ʽ  
    wfx.nChannels = 1;//������Ƶ�ļ���ͨ������  
    wfx.nSamplesPerSec = 16000;//����ÿ���������źͼ�¼ʱ������Ƶ��  
    wfx.nAvgBytesPerSec = 32000;//���������ƽ�����ݴ�����,��λbyte/s�����ֵ���ڴ��������С�Ǻ����õ�  
    wfx.nBlockAlign = 2;//���ֽ�Ϊ��λ���ÿ����  
    wfx.wBitsPerSample = 16;  
    wfx.cbSize = 0;//������Ϣ�Ĵ�С  
    wait = CreateEvent(NULL, 0, 0, NULL);  
    waveOutOpen(&hwo, WAVE_MAPPER, &wfx, (DWORD_PTR)wait, 0L, CALLBACK_EVENT);//��һ�������Ĳ�����Ƶ���װ�������лط�  
    fopen_s(&thbgm, "text_to_speech_test.pcm", "rb"); 
	int mcount=0;
	fread(buf, sizeof(struct wave_pcm_hdr), 1, thbgm);
	while(fread(buf, sizeof(char), 1024, thbgm)==1024)
	{
		mcount++;
	}
	fseek(thbgm,0,SEEK_SET);
    cnt = fread(buf, sizeof(char), 1024 * (mcount) , thbgm);//��ȡ�ļ�4M�����ݵ��ڴ������в��ţ�ͨ��������ֵ��޸ģ������߳̿ɱ��������Ƶ���ݵ�ʵʱ���䡣��Ȼ���ϣ��������������Ƶ�ļ���Ҳ��Ҫ��������΢��һ��  
    int dolenght = 0;  
    int playsize = 1024;  
    while (cnt) {//��һ������Ҫ�ر�ע�������ѭ������֮���ܻ�̫����ʱ��ȥ����ȡ����֮��Ĺ�������Ȼ��ÿ��ѭ���ļ�϶���С����ա�������  
        wh.lpData = buf + dolenght;  
        wh.dwBufferLength = playsize;  
        wh.dwFlags = 0L;  
        wh.dwLoops = 1L;  
        waveOutPrepareHeader(hwo, &wh, sizeof(WAVEHDR));//׼��һ���������ݿ����ڲ���  
        waveOutWrite(hwo, &wh, sizeof(WAVEHDR));//����Ƶý���в��ŵڶ�������whָ��������  
        WaitForSingleObject(wait, INFINITE);//�������hHandle�¼����ź�״̬����ĳһ�߳��е��øú���ʱ���߳���ʱ��������ڹ����INFINITE�����ڣ��߳����ȴ��Ķ����Ϊ���ź�״̬����ú�����������  
        dolenght = dolenght + playsize;  
        cnt = cnt - playsize;  
    }  
    waveOutClose(hwo);  
    fclose(thbgm);  
	key = _getch();//�˳�
}


void CrecordxDlg::OnBnClickedButton7()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������

	//CString text="�й��˵���ʳ����ע��ζ������ͬ�ĵ���Բ�Ʒ��ζ��Ҳ�в�ͬ��ƫ�ã�����������ϲ��";
	
	FILE *file;
	char buf[BUFLEN];
	int len=0,i=0;
	char *array[MAXLINE];

	file=fopen("�ı���¼.txt","r");//��TXST.TxT�ļ�
	
	while(fgets(buf,BUFLEN,file))//��ȡTXT���ַ�
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
	  // printf("%s\n",array[i]);//��ӡtest�ĵ����ַ�
	   m_editMultiLine.SetWindowText(_T(array[i]));
	   free(array[i--]);
	}
	
	//const char* text  = "ȴû�кû�֮�֡�";//���ϳɵ��ı�
}
