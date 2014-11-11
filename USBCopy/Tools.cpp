// Tools.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "Tools.h"
#include "afxdialogex.h"
#include "GlobalDef.h"

// CTools �Ի���

IMPLEMENT_DYNAMIC(CTools, CDialogEx)

CTools::CTools(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTools::IDD, pParent)
{
	if (pParent != NULL)
	{
		m_hParent = pParent->GetSafeHwnd();
	}
}

CTools::~CTools()
{
}

void CTools::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTools, CDialogEx)
	ON_WM_SETCURSOR()
	ON_WM_ACTIVATE()
	ON_BN_CLICKED(IDC_BTN_DISK_FORMART, &CTools::OnBnClickedBtnDiskFormart)
	ON_BN_CLICKED(IDC_BTN_FULL_RW_TEST, &CTools::OnBnClickedBtnFullRwTest)
	ON_BN_CLICKED(IDC_BTN_FAKE_PICKER, &CTools::OnBnClickedBtnFakePicker)
	ON_BN_CLICKED(IDC_BTN_CAPACITY_CHECK, &CTools::OnBnClickedBtnCapacityCheck)
	ON_BN_CLICKED(IDC_BTN_SPEED_CHECK, &CTools::OnBnClickedBtnSpeedCheck)
	ON_BN_CLICKED(IDC_BTN_BURN_IN, &CTools::OnBnClickedBtnBurnIn)
END_MESSAGE_MAP()


// CTools ��Ϣ�������


BOOL CTools::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	m_BtnDiskFormat.SubclassDlgItem(IDC_BTN_DISK_FORMART,this);
	m_BtnDiskFormat.SetFlat(FALSE);
	m_BtnDiskFormat.SetBitmaps(IDB_DISK_FORMAT,RGB(255,255,255));

	m_BtnFullRWTest.SubclassDlgItem(IDC_BTN_FULL_RW_TEST,this);
	m_BtnFullRWTest.SetFlat(FALSE);
	m_BtnFullRWTest.SetBitmaps(IDB_FULL_SCAN,RGB(255,255,255));

	m_BtnFakePicker.SubclassDlgItem(IDC_BTN_FAKE_PICKER,this);
	m_BtnFakePicker.SetFlat(FALSE);
	m_BtnFakePicker.SetBitmaps(IDB_FAKE_PICKER,RGB(255,255,255));

	m_BtnCapCheck.SubclassDlgItem(IDC_BTN_CAPACITY_CHECK,this);
	m_BtnCapCheck.SetFlat(FALSE);
	m_BtnCapCheck.SetBitmaps(IDB_CAP_CHECK,RGB(255,255,255));

	m_BtnSpeedCheck.SubclassDlgItem(IDC_BTN_SPEED_CHECK,this);
	m_BtnSpeedCheck.SetFlat(FALSE);
	m_BtnSpeedCheck.SetBitmaps(IDB_SPEED_TEST,RGB(255,255,255));

	m_BtnBurnIn.SubclassDlgItem(IDC_BTN_BURN_IN,this);
	m_BtnBurnIn.SetFlat(FALSE);
	m_BtnBurnIn.SetBitmaps(IDB_BURN_IN,RGB(255,255,255));


	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}


BOOL CTools::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (pWnd != NULL)
	{
		switch(pWnd->GetDlgCtrlID())
		{
		case IDC_BTN_DISK_FORMART:
		case IDC_BTN_FULL_RW_TEST:
		case IDC_BTN_FAKE_PICKER:
		case IDC_BTN_CAPACITY_CHECK:
		case IDC_BTN_SPEED_CHECK:
		case IDC_BTN_BURN_IN:
			if (pWnd->IsWindowEnabled())
			{
				SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_HAND)));
				return TRUE;
			}
		}
	}

	return CDialogEx::OnSetCursor(pWnd, nHitTest, message);
}


void CTools::OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized)
{
	CDialogEx::OnActivate(nState, pWndOther, bMinimized);

	// TODO: �ڴ˴������Ϣ����������
	if (nState == WA_INACTIVE)
	{
		CDialogEx::OnOK();
	}
}


void CTools::OnBnClickedBtnDiskFormart()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_WORK_MODE_SELECT,(WPARAM)WorkMode_DiskFormat,(LPARAM)WorkMode_DiskFormat);

	CDialogEx::OnOK();

}


void CTools::OnBnClickedBtnFullRwTest()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_WORK_MODE_SELECT,(WPARAM)WorkMode_Full_RW_Test,(LPARAM)WorkMode_Full_RW_Test);

	CDialogEx::OnOK();
}


void CTools::OnBnClickedBtnFakePicker()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_WORK_MODE_SELECT,(WPARAM)WorkMode_Fade_Picker,(LPARAM)WorkMode_Fade_Picker);

	CDialogEx::OnOK();
}


void CTools::OnBnClickedBtnCapacityCheck()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_WORK_MODE_SELECT,(WPARAM)WorkMode_Capacity_Check,(LPARAM)WorkMode_Capacity_Check);

	CDialogEx::OnOK();
}


void CTools::OnBnClickedBtnSpeedCheck()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_WORK_MODE_SELECT,(WPARAM)WorkMode_Speed_Check,(LPARAM)WorkMode_Speed_Check);

	CDialogEx::OnOK();
}


void CTools::OnBnClickedBtnBurnIn()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	::PostMessage(m_hParent,WM_WORK_MODE_SELECT,(WPARAM)WorkMode_Burnin_Test,(LPARAM)WorkMode_Burnin_Test);

	CDialogEx::OnOK();
}
