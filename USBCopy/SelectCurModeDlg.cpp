// SelectCurModeDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "SelectCurModeDlg.h"
#include "afxdialogex.h"
#include "Tools.h"

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
	ON_BN_CLICKED(IDC_BTN_TOOLS, &CSelectCurModeDlg::OnBnClickedBtnTools)
	ON_BN_CLICKED(IDC_BTN_RETURN, &CSelectCurModeDlg::OnBnClickedBtnReturn)
	ON_BN_CLICKED(IDC_BTN_DIFFERENCE_COPY, &CSelectCurModeDlg::OnBnClickedBtnDifferenceCopy)
	ON_WM_SHOWWINDOW()
	ON_BN_CLICKED(IDC_BTN_MTP_COPY, &CSelectCurModeDlg::OnBnClickedBtnMtpCopy)
	ON_MESSAGE(WM_WORK_MODE_SELECT, &CSelectCurModeDlg::OnWorkModeSelect)
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

	case WorkMode_DifferenceCopy:
		strResText.LoadString(IDS_WORK_MODE_DIFF_COPY);
		break;

	case WorkMode_MTPCopy:
		strResText.LoadString(IDS_WORK_MODE_MTP_COPY);
		break;

	case WorkMode_Full_RW_Test:
		strResText.LoadString(IDS_WORK_MODE_FULL_RW_TEST);
		break;

	case WorkMode_Fade_Picker:
		strResText.LoadString(IDS_WORK_MODE_FADE_PICKER);
		break;

	case WorkMode_Capacity_Check:
		strResText.LoadString(IDS_WORK_MODE_CAP_CHECK);
		break;

	case WorkMode_Speed_Check:
		strResText.LoadString(IDS_WORK_MODE_SPEED_CHECK);
		break;

	case WorkMode_Burnin_Test:
		strResText.LoadString(IDS_WORK_MODE_BURN_IN);
		break;
	}

	strMode.Format(_T(" - %s"),strResText);

	CString strTitle;
	GetWindowText(strTitle);
	strTitle += strMode;
	SetWindowText(strTitle);

	m_font.CreatePointFont(160,_T("Arial"));

	GetDlgItem(IDC_BTN_FULL_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_QUICK_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_FILE_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_IMAGE_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_IMAGE_MAKE)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_DISK_COMPARE)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_DISK_CLEAN)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_TOOLS)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_DIFFERENCE_COPY)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_RETURN)->SetFont(&m_font);
	GetDlgItem(IDC_BTN_MTP_COPY)->SetFont(&m_font);

	m_BtnFullCopy.SubclassDlgItem(IDC_BTN_FULL_COPY,this);
	m_BtnFullCopy.SetFlat(FALSE);
	m_BtnFullCopy.SetBitmaps(IDB_FULL_COPY,RGB(255,255,255));

	m_BtnQuickCopy.SubclassDlgItem(IDC_BTN_QUICK_COPY,this);
	m_BtnQuickCopy.SetFlat(FALSE);
	m_BtnQuickCopy.SetBitmaps(IDB_QUICK_COPY,RGB(255,255,255));

	m_BtnFileCopy.SubclassDlgItem(IDC_BTN_FILE_COPY,this);
	m_BtnFileCopy.SetFlat(FALSE);
	m_BtnFileCopy.SetBitmaps(IDB_FILE_COPY,RGB(255,255,255));

	m_BtnImageCopy.SubclassDlgItem(IDC_BTN_IMAGE_COPY,this);
	m_BtnImageCopy.SetFlat(FALSE);
	m_BtnImageCopy.SetBitmaps(IDB_IMAGE_COPY,RGB(255,255,255));

	m_BtnImageMake.SubclassDlgItem(IDC_BTN_IMAGE_MAKE,this);
	m_BtnImageMake.SetFlat(FALSE);
	m_BtnImageMake.SetBitmaps(IDB_MAKE_IMAGE,RGB(255,255,255));

	m_BtnDiskClean.SubclassDlgItem(IDC_BTN_DISK_CLEAN,this);
	m_BtnDiskClean.SetFlat(FALSE);
	m_BtnDiskClean.SetBitmaps(IDB_DISK_CLEAN,RGB(255,255,255));

	m_BtnDiskCompare.SubclassDlgItem(IDC_BTN_DISK_COMPARE,this);
	m_BtnDiskCompare.SetFlat(FALSE);
	m_BtnDiskCompare.SetBitmaps(IDB_DISK_COMPARE,RGB(255,255,255));

	m_BtnTools.SubclassDlgItem(IDC_BTN_TOOLS,this);
	m_BtnTools.SetFlat(FALSE);
	m_BtnTools.SetBitmaps(IDB_TOOLS,RGB(255,255,255));

	m_BtnReturn.SubclassDlgItem(IDC_BTN_RETURN,this);
	m_BtnReturn.SetBitmaps(IDB_RETURN,RGB(255,255,255));
	//m_BtnReturn.SetFlat(FALSE);
	m_BtnReturn.DrawBorder(FALSE);
	SetDlgItemText(IDC_BTN_RETURN,_T(""));

	m_BtnDiffCopy.SubclassDlgItem(IDC_BTN_DIFFERENCE_COPY,this);
	m_BtnDiffCopy.SetFlat(FALSE);
	m_BtnDiffCopy.SetBitmaps(IDB_DIFF_COPY,RGB(255,255,255));

	m_BtnMTPCopy.SubclassDlgItem(IDC_BTN_MTP_COPY,this);
	m_BtnMTPCopy.SetFlat(FALSE);
	m_BtnMTPCopy.SetBitmaps(IDB_MTP_COPY,RGB(255,255,255));

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
		case IDC_BTN_TOOLS:
		case IDC_BTN_RETURN:
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


void CSelectCurModeDlg::OnBnClickedBtnTools()
{
	// TODO: 在此添加控件通知处理程序代码
	CRect rectBtn,rectDlg;
	GetDlgItem(IDC_BTN_TOOLS)->GetWindowRect(rectBtn);

	CTools *tools = new CTools(this);
	tools->Create(IDD_DIALOG_TOOLS);
	tools->GetWindowRect(&rectDlg);

	tools->MoveWindow(rectBtn.right,rectBtn.top - rectDlg.Height() / 2,rectDlg.Width(),rectDlg.Height());
	tools->ShowWindow(SW_SHOW);
}


void CSelectCurModeDlg::OnBnClickedBtnReturn()
{
	// TODO: 在此添加控件通知处理程序代码
	OnOK();
}


void CSelectCurModeDlg::OnBnClickedBtnDifferenceCopy()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_DifferenceCopy;
	OnOK();
}


void CSelectCurModeDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialogEx::OnShowWindow(bShow, nStatus);

	// TODO: 在此处添加消息处理程序代码
	CRect rect,rectParent;
	GetWindowRect(&rect);
	GetParent()->GetClientRect(rectParent);
	GetParent()->ClientToScreen(&rectParent);

	MoveWindow(rectParent.left + 10,rectParent.top + 10,rect.Width(),rect.Height());
}


void CSelectCurModeDlg::OnBnClickedBtnMtpCopy()
{
	// TODO: 在此添加控件通知处理程序代码
	m_WorkMode = WorkMode_MTPCopy;
	OnOK();
}


afx_msg LRESULT CSelectCurModeDlg::OnWorkModeSelect(WPARAM wParam, LPARAM lParam)
{
	m_WorkMode = (WorkMode)wParam;

	OnOK();

	return 0;
}
