// nQuantCpp.cpp
//

#include "stdafx.h"
#include <iostream>
#include "nQuantCpp.h"

#include "PnnQuantizer.h"
#include "NeuQuantizer.h"
#include "WuQuantizer.h"
#include "PnnLABQuantizer.h"
#include "EdgeAwareSQuantizer.h"
#include "SpatialQuantizer.h"
#include "DivQuantizer.h"
#include "MoDEQuantizer.h"
#include "MedianCut.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef UNICODE
wostream& tcout = wcout;
#else
ostream& tcout = cout;
#endif

GdiplusStartupInput  m_gdiplusStartupInput;
ULONG_PTR m_gdiplusToken;

CString algs = _T("PNN, PNNLAB, NEU, WU, EAS, SPA, DIV, MODE, MMC");

void PrintUsage()
{
    cout << endl;
    cout << "usage: nQuantCpp <input image path> [options]" << endl;
    cout << endl;
    cout << "Valid options:" << endl;
	cout << "  /a : Algorithm used - Choose one of them, otherwise give you the defaults from [" << CStringA(algs) << "] ." << endl;
    cout << "  /m : Max Colors (pixel-depth) - Maximum number of colors for the output format to support. The default is 256 (8-bit)." << endl;
    cout << "  /o : Output image file dir. The default is <source image path directory>" << endl;
}

bool isdigit(const char* string) {
	const int string_len = strlen(string);
	for (int i = 0; i < string_len; ++i) {
		if (!isdigit(string[i]))
			return false;
	}
	return true;
}

bool isAlgo(const CString& algo) {
	int nTokenPos = 0;
	CString strToken = algs.Tokenize(_T(", "), nTokenPos);

	while (!strToken.IsEmpty()) {
		if (strToken == algo)
			return true;
		strToken = algs.Tokenize(_T(", "), nTokenPos);
	}
	return false;
}

bool ProcessArgs(int argc, CString& algo, UINT& nMaxColors, CString& targetPath, char** argv)
{
	for (int index = 1; index < argc; ++index) {
		auto currentArg = CString(argv[index]).MakeUpper();
		auto currentCmd = currentArg[0];
		if (currentArg.GetLength() > 1 && 
			(currentCmd == _T('-') || currentCmd == _T('–') || currentCmd == _T('/'))) {
			if (currentArg[1] == _T('A')) {
				CString tmpAlgo = CString(argv[index + 1]);
				if (index >= argc - 1 || !isAlgo(tmpAlgo)) {
					PrintUsage();
					return false;
				}
				algo = tmpAlgo;
			}
			else if (currentArg[1] == _T('M')) {
				if (index >= argc - 1 || !isdigit(argv[index + 1])) {
					PrintUsage();
					return false;
				}
				nMaxColors = atoi(argv[index + 1]);
				if (nMaxColors < 2)
					nMaxColors = 2;
				else if (nMaxColors > 65536)
					nMaxColors = 65536;
			}
			else if (currentArg[1] == _T('O')) {
				if (index >= argc - 1) {
					PrintUsage();
					return false;
				}
				targetPath = CString(argv[index + 1]);
			}
			else {
				PrintUsage();
				return false;
			}
		}
	}
	return true;
}

bool QuantizeImage(const CString& algorithm, LPCTSTR sourceFile, LPCTSTR targetDir, Bitmap* pSource, UINT nMaxColors, bool dither)
{	
	// Create 8 bpp indexed bitmap of the same size
	auto pDest = make_unique<Bitmap>(pSource->GetWidth(), pSource->GetHeight(), (nMaxColors > 256) ? PixelFormat16bppARGB1555 : (nMaxColors > 16) ? PixelFormat8bppIndexed : (nMaxColors > 2) ? PixelFormat4bppIndexed : PixelFormat1bppIndexed);

	bool bSucceeded = false;
	if(algorithm == _T("PNN")) {
		PnnQuant::PnnQuantizer pnnQuantizer;
		bSucceeded = pnnQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors, dither);
	}
	else if(algorithm == _T("PNNLAB")) {
		PnnLABQuant::PnnLABQuantizer pnnLABQuantizer;
		bSucceeded = pnnLABQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors, dither);
	}
	else if(algorithm == _T("NEU")) {
		NeuralNet::NeuQuantizer neuQuantizer;
		bSucceeded = neuQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors, dither);
	}
	else if(algorithm == _T("WU")) {
		nQuant::WuQuantizer wuQuantizer;
		bSucceeded = wuQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors, dither);
	}
	else if(algorithm == _T("EAS")) {
		EdgeAwareSQuant::EdgeAwareSQuantizer easQuantizer;
		bSucceeded = easQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors);
	}
	else if(algorithm == _T("SPA")) {
		SpatialQuant::SpatialQuantizer spaQuantizer;
		bSucceeded = spaQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors);
	}
	else if (algorithm == _T("DIV")) {
		DivQuant::DivQuantizer divQuantizer;
		bSucceeded = divQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors, dither);
	}
	else if (algorithm == _T("MODE")) {
		MoDEQuant::MoDEQuantizer moDEQuantizer;
		bSucceeded = moDEQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors, dither);		
	}
	else if (algorithm == _T("MMC")) {
		MedianCutQuant::MedianCut mmcQuantizer;
		bSucceeded = mmcQuantizer.QuantizeImage(pSource, pDest.get(), nMaxColors, dither);
	}
	
	if(!bSucceeded)
		return bSucceeded;

	TCHAR fileName[MAX_PATH];
	_tcsncpy_s(fileName, sourceFile, MAX_PATH);
	PathRemoveExtension(fileName);

	TCHAR targetPath[MAX_PATH];
	PathCombine(targetPath, targetDir, fileName);
	CString destPath;
	destPath.Format(_T("%s-%squant%d.png"), targetPath, (LPCTSTR) algorithm, nMaxColors);
	
	// image/png  : {557cf406-1a04-11d3-9a73-0000f81ef32e}
	const CLSID pngEncoderClsId = { 0x557cf406, 0x1a04, 0x11d3,{ 0x9a,0x73,0x00,0x00,0xf8,0x1e,0xf3,0x2e } };
	Status status = pDest->Save(destPath, &pngEncoderClsId);
	if (status == Status::Ok)
		tcout << _T("Converted image: ") << (LPCTSTR) destPath << endl;
	else
		tcout << _T("Failed to save image in '") << (LPCTSTR) destPath << _T("' file") << endl;

	return status == Status::Ok;
}

int main(int argc, char** argv)
{
	if (argc <= 1) {
#ifndef _DEBUG
		PrintUsage();
		return 0;
#endif
	}
	
	TCHAR szDirectory[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, szDirectory);
	CString szDir = szDirectory;	
	
	UINT nMaxColors = 256;	
	CString algo = _T(""), targetDir = _T("");
#ifdef _DEBUG
	CString sourcePath = szDir + _T("\\..\\ImgV64.gif");
	nMaxColors = 1024;
#else
	if (!ProcessArgs(argc, algo, nMaxColors, targetDir, argv))
		return 0;

	CString sourcePath = CString(argv[1]);
	if (sourcePath.FindOneOf(_T("\\")) < 0) {
		TCHAR szSourcePath[MAX_PATH];
		PathCombine(szSourcePath, szDir, sourcePath);
		sourcePath = szSourcePath;
	}
#endif	
	
	if(!PathFileExists(sourcePath)) {
		cout << "The source file you specified does not exist." << endl;
		return 0;
	}		

	if(GdiplusStartup(&m_gdiplusToken, &m_gdiplusStartupInput, NULL) == Ok) {
		auto pSource = unique_ptr<Bitmap>(Bitmap::FromFile(sourcePath));
		Status status = pSource->GetLastStatus();
		if (status == Ok) {
			if (!PathFileExists(targetDir))
				targetDir = szDir.Left(sourcePath.ReverseFind(_T('\\')));

			bool dither = true;
			CString sourceFile = sourcePath.Mid(sourcePath.ReverseFind(_T('\\')) + 1);
			if (algo == _T("")) {
				//QuantizeImage(_T("MMC"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
				QuantizeImage(_T("DIV"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
				if (nMaxColors > 32) {
					QuantizeImage(_T("PNN"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
					QuantizeImage(_T("WU"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
					//QuantizeImage(_T("MODE"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
					QuantizeImage(_T("NEU"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
				}
				else {
					QuantizeImage(_T("PNNLAB"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
					QuantizeImage(_T("EAS"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
					QuantizeImage(_T("SPA"), sourceFile, targetDir, pSource.get(), nMaxColors, dither);
				}
			}
			else
				QuantizeImage(algo, sourceFile, targetDir, pSource.get(), nMaxColors, dither);
		}
		else
			tcout << _T("Failed to read image in '") << (LPCTSTR) sourcePath << _T("' file");
	}
	GdiplusShutdown(m_gdiplusToken);
    return 0;
}
