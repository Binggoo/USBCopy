// LoadConfig.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "USBCopy.h"
#include "LoadConfig.h"
#include "afxdialogex.h"
#include "Utils.h"

// CLoadConfig �Ի���

IMPLEMENT_DYNAMIC(CLoadConfig, CDialogEx)

CLoadConfig::CLoadConfig(CWnd* pParent /*=NULL*/)
	: CDialogEx(CLoadConfig::IDD, pParent)
{
}

CLoadConfig::~CLoadConfig()
{
}

void CLoadConfig::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CONFIG, m_ListCtrl);
}


BEGIN_MESSAGE_MAP(CLoadConfig, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_DEL, &CLoadConfig::OnBnClickedBtnDel)
	ON_BN_CLICKED(IDOK, &CLoadConfig::OnBnClickedOk)
END_MESSAGE_MAP()


// CLoadConfig ��Ϣ�������


BOOL CLoadConfig::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  �ڴ���Ӷ���ĳ�ʼ��
	CString strPath = CUtils::GetAppPath();
	m_strAppPath = CUtils::GetFilePathWithoutName(strPath);

	InitialListCtrl();

	AddListData();

	return TRUE;  // return TRUE unless you set the focus to a control
	// �쳣: OCX ����ҳӦ���� FALSE
}

void CLoadConfig::InitialListCtrl()
{
	// ��ʼ��ListCtrl
	CRect rect;
	m_ListCtrl.GetClientRect(&rect);
	int nWidth = rect.Width();

	CString strResText;

	int nItem = 0;

	strResText.LoadString(IDS_ITEM_FILE_NAME);
	m_ListCtrl.InsertColumn(nItem++,strResText,LVCFMT_LEFT,nWidth,0);

	m_ListCtrl.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
}

void CLoadConfig::AddListData()
{
	m_ListCtrl.DeleteAllItems();

	// ��ԭʼ�ļ������б�
	CString strFindFile = m_strAppPath + CONFIG_NAME;
	CString strFileName;
	strFileName = CUtils::GetFileName(strFindFile);

	int nItem = 0;
	m_ListCtrl.InsertItem(nItem,strFileName);
	nItem++;

	strFindFile.Format(_T("%s\\config\\*.ini"),m_strAppPath);

	CFileFind ff;
	BOOL bFind = ff.FindFile(strFindFile,0);
	while (bFind)
	{
		bFind = ff.FindNextFile();

		strFileName = ff.GetFileName();
	
		m_ListCtrl.InsertItem(nItem,strFileName);
		nItem++;
		
	}
	ff.Close();
}


void CLoadConfig::OnBnClickedBtnDel()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	POSITION pos = m_ListCtrl.GetFirstSelectedItemPosition();

	while(pos)
	{
		int nItem = m_ListCtrl.GetNextSelectedItem(pos);
		CString strFindFile;
		strFindFile.Format(_T("%s\\config\\%s"),m_strAppPath,m_ListCtrl.GetItemText(nItem,0));

		if (DeleteFile(strFindFile))
		{
			m_ListCtrl.DeleteItem(nItem);
			pos = m_ListCtrl.GetFirstSelectedItemPosition();
		}

	}
}


void CLoadConfig::OnBnClickedOk()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	POSITION pos = m_ListCtrl.GetFirstSelectedItemPosition();

	if (pos)
	{
		int nItem = m_ListCtrl.GetNextSelectedItem(pos);

		CString strFindFile;

		if (nItem == 0)
		{
			strFindFile = m_strAppPath + CONFIG_NAME;
		}
		else
		{
			strFindFile.Format(_T("%s\\config\\%s"),m_strAppPath,m_ListCtrl.GetItemText(nItem,0));
		}
		

		if (PathFileExists(strFindFile))
		{
			m_strConfigName = strFindFile;

			CDialogEx::OnOK();
		}
		
	}
}

CString CLoadConfig::GetPathName()
{
	return m_strConfigName;
}
