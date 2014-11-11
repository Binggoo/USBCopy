// BurnIn.cpp : 实现文件
//

#include "stdafx.h"
#include "USBCopy.h"
#include "BurnIn.h"
#include "afxdialogex.h"


// CBurnIn 对话框

IMPLEMENT_DYNAMIC(CBurnIn, CDialogEx)

CBurnIn::CBurnIn(CWnd* pParent /*=NULL*/)
	: CDialogEx(CBurnIn::IDD, pParent)
{
	m_pIni = NULL;
}

CBurnIn::~CBurnIn()
{
}

void CBurnIn::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_CYCLE_COUNT, m_ComboBoxCycleCount);
	DDX_Control(pDX, IDC_LIST_FUNCTION, m_ListBoxFunction);
	DDX_Control(pDX, IDC_COMBO_FUNCTION, m_ComboBoxFunction);
}


BEGIN_MESSAGE_MAP(CBurnIn, CDialogEx)
	ON_WM_SETCURSOR()
	ON_BN_CLICKED(IDC_BTN_DEL, &CBurnIn::OnBnClickedBtnDel)
	ON_BN_CLICKED(IDC_BTN_UP, &CBurnIn::OnBnClickedBtnUp)
	ON_BN_CLICKED(IDC_BTN_DOWN, &CBurnIn::OnBnClickedBtnDown)
	ON_BN_CLICKED(IDC_BTN_ADD, &CBurnIn::OnBnClickedBtnAdd)
	ON_BN_CLICKED(IDOK, &CBurnIn::OnBnClickedOk)
	ON_CBN_EDITCHANGE(IDC_COMBO_CYCLE_COUNT, &CBurnIn::OnCbnEditchangeComboCycleCount)
END_MESSAGE_MAP()


// CBurnIn 消息处理程序


BOOL CBurnIn::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化
	ASSERT(m_pIni);

	m_ComboBoxCycleCount.AddString(_T("1"));
	m_ComboBoxCycleCount.AddString(_T("2"));
	m_ComboBoxCycleCount.AddString(_T("3"));
	m_ComboBoxCycleCount.AddString(_T("4"));
	m_ComboBoxCycleCount.AddString(_T("5"));
	m_ComboBoxCycleCount.AddString(_T("10"));
	m_ComboBoxCycleCount.AddString(_T("20"));
	m_ComboBoxCycleCount.AddString(_T("30"));
	m_ComboBoxCycleCount.AddString(_T("40"));
	m_ComboBoxCycleCount.AddString(_T("50"));
	
	int nItem = 0;
	CString strResText;
	strResText.LoadString(IDS_WORK_MODE_FULL_COPY);
	nItem = m_ComboBoxFunction.AddString(strResText);
	m_ComboBoxFunction.SetItemData(nItem,WorkMode_FullCopy);

	strResText.LoadString(IDS_WORK_MODE_QUICK_COPY);
	nItem = m_ComboBoxFunction.AddString(strResText);
	m_ComboBoxFunction.SetItemData(nItem,WorkMode_QuickCopy);

	strResText.LoadString(IDS_WORK_MODE_FILE_COPY);
	nItem = m_ComboBoxFunction.AddString(strResText);
	m_ComboBoxFunction.SetItemData(nItem,WorkMode_FileCopy);

	strResText.LoadString(IDS_WORK_MODE_IMAGE_COPY);
	nItem = m_ComboBoxFunction.AddString(strResText);
	m_ComboBoxFunction.SetItemData(nItem,WorkMode_ImageCopy);

	strResText.LoadString(IDS_WORK_MODE_IMAGE_MAKE);
	nItem = m_ComboBoxFunction.AddString(strResText);
	m_ComboBoxFunction.SetItemData(nItem,WorkMode_ImageMake);

	strResText.LoadString(IDS_WORK_MODE_DISK_CLEAN);
	nItem = m_ComboBoxFunction.AddString(strResText);
	m_ComboBoxFunction.SetItemData(nItem,WorkMode_DiskClean);

	strResText.LoadString(IDS_WORK_MODE_DISK_COMPARE);
	nItem = m_ComboBoxFunction.AddString(strResText);
	m_ComboBoxFunction.SetItemData(nItem,WorkMode_DiskCompare);

	strResText.LoadString(IDS_WORK_MODE_DISK_FORMAT);
	nItem = m_ComboBoxFunction.AddString(strResText);
	m_ComboBoxFunction.SetItemData(nItem,WorkMode_DiskFormat);

	m_ComboBoxFunction.SetCurSel(0);

	UINT nCycleCount = m_pIni->GetUInt(_T("BurnIn"),_T("CycleCount"),1);
	UINT nFunctionNum = m_pIni->GetUInt(_T("BurnIn"),_T("FunctionNum"),0);

	// 初始化ListBox
	CString strKey,strFunction;
	for (UINT i = 0;i < nFunctionNum;i++)
	{
		strKey.Format(_T("Function_%d"),i);
		WorkMode mode = (WorkMode)m_pIni->GetUInt(_T("BurnIn"),strKey,0);

		strFunction = GetWorkModeString(mode);

		nItem = m_ListBoxFunction.AddString(strFunction);
		m_ListBoxFunction.SetItemData(nItem,mode);
	}

	CString strCycleCount;
	strCycleCount.Format(_T("%d"),nCycleCount);
	m_ComboBoxCycleCount.SetWindowText(strCycleCount);

	UpdateData(FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	// 异常: OCX 属性页应返回 FALSE
}


BOOL CBurnIn::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (pWnd != NULL)
	{
		switch(pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_DEL:
		case IDC_BTN_UP:
		case IDC_BTN_DOWN:
		case IDC_BTN_ADD:
		case IDCANCEL:
		case IDOK:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}


void CBurnIn::OnBnClickedBtnDel()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSelectIndex = m_ListBoxFunction.GetCurSel();
	if (nSelectIndex == LB_ERR )
	{
		return;
	}

	m_ListBoxFunction.DeleteString(nSelectIndex);

	m_ListBoxFunction.SetCurSel(nSelectIndex);
}


void CBurnIn::OnBnClickedBtnUp()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSelectIndex = m_ListBoxFunction.GetCurSel();
	if (nSelectIndex == LB_ERR || nSelectIndex == 0)
	{
		return;
	}

	CString strFunction;
	m_ListBoxFunction.GetText(nSelectIndex,strFunction);
	WorkMode mode = (WorkMode)m_ListBoxFunction.GetItemData(nSelectIndex);

	m_ListBoxFunction.DeleteString(nSelectIndex);

	m_ListBoxFunction.InsertString(nSelectIndex-1,strFunction);
	m_ListBoxFunction.SetItemData(nSelectIndex-1,mode);

	m_ListBoxFunction.SetCurSel(nSelectIndex-1);
	
}


void CBurnIn::OnBnClickedBtnDown()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSelectIndex = m_ListBoxFunction.GetCurSel();
	int nCount = m_ListBoxFunction.GetCount();
	if (nSelectIndex == LB_ERR || nSelectIndex == nCount-1)
	{
		return;
	}

	CString strFunction;
	m_ListBoxFunction.GetText(nSelectIndex,strFunction);
	WorkMode mode = (WorkMode)m_ListBoxFunction.GetItemData(nSelectIndex);

	m_ListBoxFunction.DeleteString(nSelectIndex);

	m_ListBoxFunction.InsertString(nSelectIndex+1,strFunction);
	m_ListBoxFunction.SetItemData(nSelectIndex+1,mode);

	m_ListBoxFunction.SetCurSel(nSelectIndex+1);
}


void CBurnIn::OnBnClickedBtnAdd()
{
	// TODO: 在此添加控件通知处理程序代码
	int nSelectIndex = m_ComboBoxFunction.GetCurSel();
	CString strFunction;
	m_ComboBoxFunction.GetWindowText(strFunction);

	WorkMode mode = (WorkMode)m_ComboBoxFunction.GetItemData(nSelectIndex);

	int nItem = m_ListBoxFunction.AddString(strFunction);
	m_ListBoxFunction.SetItemData(nItem,mode);
}


void CBurnIn::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	int nCount = m_ListBoxFunction.GetCount();

	if (nCount == 0)
	{
		CString strResText;
		strResText.LoadString(IDS_MSG_NO_BURN_IN_FUNCTION);
		MessageBox(strResText);
		return;
	}

	CString strCycleCount;
	m_ComboBoxCycleCount.GetWindowText(strCycleCount);

	m_pIni->WriteString(_T("BurnIn"),_T("CycleCount"),strCycleCount);
	
	UINT nFunctionNum = m_pIni->GetUInt(_T("BurnIn"),_T("FunctionNum"),0);
	m_pIni->WriteUInt(_T("BurnIn"),_T("FunctionNum"),nCount);

	CString strKey;
	for (UINT i = 0;i < nFunctionNum;i++)
	{
		strKey.Format(_T("Function_%d"),i);
		m_pIni->DeleteKey(_T("BurnIn"),strKey);
	}

	for (int i = 0;i < nCount;i++)
	{
		strKey.Format(_T("Function_%d"),i);
		WorkMode mode = (WorkMode)m_ListBoxFunction.GetItemData(i);

		m_pIni->WriteUInt(_T("BurnIn"),strKey,mode);
	}
	

	CDialogEx::OnOK();
}

CString CBurnIn::GetWorkModeString( WorkMode workMode )
{
	CString strWorkMode;
	switch (workMode)
	{
	case WorkMode_FullCopy:
		strWorkMode.LoadString(IDS_WORK_MODE_FULL_COPY);
		break;
	case WorkMode_QuickCopy:
		strWorkMode.LoadString(IDS_WORK_MODE_QUICK_COPY);
		break;
	case WorkMode_FileCopy:
		strWorkMode.LoadString(IDS_WORK_MODE_FILE_COPY);
		break;
	case WorkMode_ImageCopy:
		strWorkMode.LoadString(IDS_WORK_MODE_IMAGE_COPY);
		break;
	case WorkMode_ImageMake:
		strWorkMode.LoadString(IDS_WORK_MODE_IMAGE_MAKE);
		break;
	case WorkMode_DiskClean:
		strWorkMode.LoadString(IDS_WORK_MODE_DISK_CLEAN);
		break;

	case WorkMode_DiskCompare:
		strWorkMode.LoadString(IDS_WORK_MODE_DISK_COMPARE);
		break;

	case WorkMode_DiskFormat:
		strWorkMode.LoadString(IDS_WORK_MODE_DISK_FORMAT);
		break;

	case WorkMode_DifferenceCopy:
		strWorkMode.LoadString(IDS_WORK_MODE_DIFF_COPY);
		break;
	}

	return strWorkMode;
}


void CBurnIn::OnCbnEditchangeComboCycleCount()
{
	// TODO: 在此添加控件通知处理程序代码
	CString strText;
	m_ComboBoxCycleCount.GetWindowText(strText);

	if (!_istdigit(strText.GetAt(0)))
	{
		m_ComboBoxCycleCount.SetWindowText(_T(""));
	}
}

void CBurnIn::SetConfig( CIni *pIni )
{
	m_pIni = pIni;
}
