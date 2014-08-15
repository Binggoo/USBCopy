// SelectCurModeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "SelectCurModeDlg.h"
#include "afxdialogex.h"


// CSelectCurModeDlg 对话框

IMPLEMENT_DYNAMIC(CSelectCurModeDlg, CDialogEx)

CSelectCurModeDlg::CSelectCurModeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSelectCurModeDlg::IDD, pParent)
{
	m_pIni = NULL;
}

CSelectCurModeDlg::~CSelectCurModeDlg()
{
}

void CSelectCurModeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CSelectCurModeDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_FULL_COPY, &CSelectCurModeDlg::OnBnClickedButtonFullCopy)
	ON_BN_CLICKED(IDC_BTN_QUICK_COPY, &CSelectCurModeDlg::OnBnClickedButtonQuickCopy)
	ON_BN_CLICKED(IDC_BTN_DISK_CLEAN, &CSelectCurModeDlg::OnBnClickedButtonDiskClean)
	ON_BN_CLICKED(IDC_BTN_IMAGE_MAKE, &CSelectCurModeDlg::OnBnClickedButtonMakeImage)
	ON_BN_CLICKED(IDC_BTN_IMAGE_COPY, &CSelectCurModeDlg::OnBnClickedButtonCopyFromImage)
	ON_BN_CLICKED(IDC_BTN_FILE_COPY, &CSelectCurModeDlg::OnBnClickedBtnFileCopy)
	ON_BN_CLICKED(IDC_BTN_DISK_COMPARE, &CSelectCurModeDlg::OnBnClickedBtnDiskCompare)
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDC_BTN_DISK_FORMAT, &CSelectCurModeDlg::OnBnClickedBtnDiskFormat)
END_MESSAGE_MAP()


// CSelectCurModeDlg 消息处理程序

void CSelectCurModeDlg::OnBnClickedButtonFullCopy()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_FullCopy;
	OnOK();
}


void CSelectCurModeDlg::OnBnClickedButtonQuickCopy()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_QuickCopy;
	OnOK();
}


void CSelectCurModeDlg::OnBnClickedButtonDiskClean()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_DiskClean;
	OnOK();
}


void CSelectCurModeDlg::OnBnClickedButtonMakeImage()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_ImageMake;
	OnOK();
}


void CSelectCurModeDlg::OnBnClickedButtonCopyFromImage()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_ImageCopy;
	OnOK();
}

void CSelectCurModeDlg::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}


void CSelectCurModeDlg::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类
	m_pIni->WriteUInt(_T("Option"),_T("FunctionMode"),(UINT)m_WorkMode);
	CDialogEx::OnOK();
}


BOOL CSelectCurModeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);
	m_WorkMode = (WorkMode)m_pIni->GetUInt(_T("Option"),_T("FunctionMode"),1);

	CString strMode,strResText;
	switch (m_WorkMode)
	{
	case WorkMode_FullCopy:
		strResText.LoadString(IDS_WORK_MODE_FULL_COPY);
		
		break;

	case WorkMode_QuickCopy:
		strResText.LoadString(IDS_WORK_MODE_QUICK_COPY);
		break;

	case WorkMode_DiskClean:
		strResText.LoadString(IDS_WORK_MODE_DISK_CLEAN);
		break;

	case WorkMode_ImageMake:
		strResText.LoadString(IDS_WORK_MODE_IMAGE_MAKE);
		break;

	case WorkMode_ImageCopy:
		strResText.LoadString(IDS_WORK_MODE_IMAGE_COPY);
		break;

	case WorkMode_FileCopy:
		strResText.LoadString(IDS_WORK_MODE_FILE_COPY);
		break;

	case WorkMode_DiskCompare:
		strResText.LoadString(IDS_WORK_MODE_DISK_COMPARE);
		break;

	case WorkMode_DiskFormat:
		strResText.LoadString(IDS_WORK_MODE_DISK_FORMAT);
		break;
	}

	strMode.Format(_T(" - %s"),strResText);

	CString strTitle;
	GetWindowText(strTitle);
	strTitle += strMode;
	SetWindowText(strTitle);

	m_font.CreatePointFont(200,_T("Arial"));

	GetDlgItem(IDC_BTN_FULL_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_QUICK_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_FILE_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_IMAGE_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_IMAGE_MAKE)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_DISK_COMPARE)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_DISK_CLEAN)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_DISK_FORMAT)->SetFont(&m_font);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


void CSelectCurModeDlg::OnBnClickedBtnFileCopy()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_FileCopy;
	OnOK();
}


void CSelectCurModeDlg::OnBnClickedBtnDiskCompare()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_DiskCompare;
	OnOK();
}


BOOL CSelectCurModeDlg::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pWnd != NULL)
	{
		switch (pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_FULL_COPY:
		case IDC_BTN_QUICK_COPY:
		case IDC_BTN_IMAGE_COPY:
		case IDC_BTN_FILE_COPY:
		case IDC_BTN_IMAGE_MAKE:
		case IDC_BTN_DISK_COMPARE:
		case IDC_BTN_DISK_CLEAN:
		case IDC_BTN_DISK_FORMAT:
		//case IDOK:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}
	

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}


void CSelectCurModeDlg::OnBnClickedBtnDiskFormat()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_DiskFormat;
	OnOK();
}
