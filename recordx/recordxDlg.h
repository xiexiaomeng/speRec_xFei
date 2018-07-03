
// recordxDlg.h : 头文件
//
#include<mmsystem.h>
#include "afxwin.h"
//#define INP_BUFFER_SIZE 16*1024
#define INP_BUFFER_SIZE 16*1024
#pragma once


// CrecordxDlg 对话框
class CrecordxDlg : public CDialogEx
{
// 构造
public:
	CrecordxDlg(CWnd* pParent = NULL);	// 标准构造函数

	////////数据成员//////////
	DWORD m_dwDataLength;        //数据长度
	WAVEFORMATEX m_waveform;    //声音格式
	HWAVEIN m_hWaveIn;            //音频输入句柄
	HWAVEOUT m_hWaveOut;        //音频输出句柄
	PBYTE m_pSaveBuffer;        //音频存储内存

	WAVEHDR m_WAVEHDR1;
	WAVEHDR m_WAVEHDR2;

	enum{ RECORD_STATE_INIT,
		RECORD_STATE_RECING,
		RECORD_STATE_STOP,
	    RECORD_STATE_STOPING,
	    RECORD_STATE_PLAYING} 
	m_nRecordState;

	char m_cbBuffer1[INP_BUFFER_SIZE];    //声音临时缓存1
	char m_cbBuffer2[INP_BUFFER_SIZE];    //声音临时缓存2

// 对话框数据
	enum { IDD = IDD_RECORDX_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();

	afx_msg void OnBnClickedButton1();	
	afx_msg void OnBnClickedButton2();
	afx_msg void OnBnClickedButton3();
	afx_msg LRESULT OnMM_WIM_OPEN(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WIM_DATA(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WIM_CLOSE(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WON_OPEN(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WOM_DONE(UINT wParam,LONG lParam);
	afx_msg LRESULT OnMM_WOM_CLOSE(UINT wParam,LONG lParam);	

	DECLARE_MESSAGE_MAP()
public:

	afx_msg void OnBnClickedButton4();
	afx_msg void OnBnClickedButton5();
	afx_msg void OnEnChangeEdit1();
	//CEdit m_editMultiLine;
	afx_msg void OnBnClickedButton6();
	afx_msg void OnBnClickedButton7();
};
