#include <stdlib.h>     /* atof, atoi */
#include <algorithm>    // std::find
#include <iostream>

#include <TH1.h>

#include "PixTestTrim.hh"
#include "log.h"


using namespace std;
using namespace pxar;

ClassImp(PixTestTrim)

// ----------------------------------------------------------------------
PixTestTrim::PixTestTrim(PixSetup *a, std::string name) : PixTest(a, name), fParVcal(-1), fParNtrig(-1), fParVthrCompLo(-1), fParVthrCompHi(-1), fParVcalLo(-1), fParVcalHi(-1) {
  PixTest::init();
  init(); 
  //  LOG(logINFO) << "PixTestTrim ctor(PixSetup &a, string, TGTab *)";
  for (unsigned int i = 0; i < fPIX.size(); ++i) {
    LOG(logDEBUG) << "  setting fPIX" << i <<  " ->" << fPIX[i].first << "/" << fPIX[i].second;
  }
}


//----------------------------------------------------------
PixTestTrim::PixTestTrim() : PixTest() {
  //  LOG(logINFO) << "PixTestTrim ctor()";
}

// ----------------------------------------------------------------------
bool PixTestTrim::setParameter(string parName, string sval) {
  bool found(false);
  for (unsigned int i = 0; i < fParameters.size(); ++i) {
    if (fParameters[i].first == parName) {
      found = true; 
      sval.erase(remove(sval.begin(), sval.end(), ' '), sval.end());
      if (!parName.compare("Ntrig")) {
	fParNtrig = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParNtrig  ->" << fParNtrig << "<- from sval = " << sval;
      }
      if (!parName.compare("Vcal")) {
	fParVcal = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcal  ->" << fParVcal << "<- from sval = " << sval;
      }
      if (!parName.compare("VcalLo")) {
	fParVcalLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcalLo  ->" << fParVcalLo << "<- from sval = " << sval;
      }
      if (!parName.compare("VcalHi")) {
	fParVcalHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVcalHi  ->" << fParVcalHi << "<- from sval = " << sval;
      }
      if (!parName.compare("VthrCompLo")) {
	fParVthrCompLo = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVthrCompLo  ->" << fParVthrCompLo << "<- from sval = " << sval;
      }
      if (!parName.compare("VthrCompHi")) {
	fParVthrCompHi = atoi(sval.c_str()); 
	LOG(logDEBUG) << "  setting fParVthrCompHi  ->" << fParVthrCompHi << "<- from sval = " << sval;
      }

      break;
    }
  }
  
  return found; 
}


// ----------------------------------------------------------------------
void PixTestTrim::init() {
  setToolTips(); 
  fDirectory = gFile->GetDirectory(fName.c_str()); 
  if (!fDirectory) {
    fDirectory = gFile->mkdir(fName.c_str()); 
  } 
  fDirectory->cd(); 

}


// ----------------------------------------------------------------------
void PixTestTrim::runCommand(std::string command) {
  std::transform(command.begin(), command.end(), command.begin(), ::tolower);
  LOG(logDEBUG) << "running command: " << command;
  if (!command.compare("trimbits")) {
    trimBitTest(); 
    return;
  }
  LOG(logDEBUG) << "did not find command ->" << command << "<-";
}


// ----------------------------------------------------------------------
void PixTestTrim::setToolTips() {
  fTestTip    = string(Form("trimming results in a uniform in-time threshold\n")
		       + string("TO BE FINISHED!!"))
    ;
  fSummaryTip = string("summary plot to be implemented")
    ;
}

// ----------------------------------------------------------------------
void PixTestTrim::bookHist(string name) {
  fDirectory->cd(); 

  LOG(logDEBUG) << "nothing done with " << name;
}


//----------------------------------------------------------
PixTestTrim::~PixTestTrim() {
  LOG(logDEBUG) << "PixTestTrim dtor";
}


// ----------------------------------------------------------------------
void PixTestTrim::doTest() {
  fDirectory->cd();
  PixTest::update(); 
  LOG(logINFO) << "PixTestTrim::doTest() ntrig = " << fParNtrig << " vcal = " << fParVcal;

  fPIX.clear(); 
  fApi->_dut->testAllPixels(false);
  fApi->_dut->maskAllPixels(true);
  sparseRoc(10); 
  fApi->setDAC("ctrlreg", 0);
  fApi->setDAC("Vcal", fParVcal);

  
  // -- determine minimal VthrComp 
  vector<int> maxVthrComp = getMaximumVthrComp(10, 0.8, 10); 
  LOG(logINFO) << "TRIM determine minimal VthrComp"; 
  vector<TH1*> thr0 = thrMaps("vthrcomp", "TrimThr0", fParNtrig); 

  vector<int> VthrComp; 
  TH2D* h(0); 
  int iroc(-1); 
  string::size_type s1, s2; 
  string hname(""); 
  for (unsigned int i = 0; i < thr0.size(); ++i) {

    hname = thr0[i]->GetName();
    s1 = hname.find("_C");
    s2 = hname.find("_V");
    iroc = atoi(hname.substr(s1+1, s2).c_str()); 

    // initialize trim bits to middle
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 80; ++iy) {
	fTrimBits[iroc][ix][iy] = 7; 
      }
    }
    

    // LOG(logDEBUG) << "   " << hname << " with ROC ID = " << iroc;
    h = (TH2D*)thr0[i]; 
    double minThr(999.); 
    int ix(-1), iy(-1); 
    for (int ic = 0; ic < h->GetNbinsX(); ++ic) {
      for (int ir = 0; ir < h->GetNbinsY(); ++ir) {
	if (h->GetBinContent(ic+1, ir+1) > 1e-5 && h->GetBinContent(ic+1, ir+1) < minThr) {
	  minThr = h->GetBinContent(ic+1, ir+1);
	  ix = ic; 
	  iy = ir; 
	}
      }
    }
    if (minThr < maxVthrComp[i]) {
      LOG(logINFO) << " XX roc " << i << " with ID = " << iroc << "  has minimal VthrComp threshold " << minThr << " for pixel " << ix << "/" << iy;
    } else {
      LOG(logINFO) << " XX roc " << i << " with ID = " << iroc << "  has minimal VthrComp threshold " << minThr 
		   << " below maximum allowed " << maxVthrComp[i] << ", resetting";
      minThr = maxVthrComp[i];
    }
    VthrComp.push_back(minThr); 
    fApi->setDAC("VthrComp", minThr, iroc); 
  }

  // FIXME do the ROCs in parallel!
  // -- determine maximal VCAL and use it to set VTRIM
  LOG(logINFO) << "TRIM determine highest Vcal "; 
  vector<TH1*> thr1 = thrMaps("vcal", "TrimThr1", fParNtrig); 
  vector<int> Vcal; 
  for (unsigned int i = 0; i < thr1.size(); ++i) {
    hname = thr1[i]->GetName();
    s1 = hname.find("_C");
    s2 = hname.find("_V");
    iroc = atoi(hname.substr(s1+1, s2).c_str()); 
    h = (TH2D*)thr1[i]; 
    double maxVcal(-1.); 
    int ix(-1), iy(-1); 
    for (int ic = 0; ic < h->GetNbinsX(); ++ic) {
      for (int ir = 0; ir < h->GetNbinsY(); ++ir) {
	if (h->GetBinContent(ic+1, ir+1) > 1e-5 && h->GetBinContent(ic+1, ir+1) > maxVcal) {
	  maxVcal = h->GetBinContent(ic+1, ir+1);
	  ix = ic; 
	  iy = ir; 
	}
      }
    }
    LOG(logINFO) << " XX roc " << i << " with ID = " << iroc << "  has maximal Vcal " << maxVcal << " for pixel " << ix << "/" << iy;
    Vcal.push_back(maxVcal); 
    fApi->setDAC("Vcal", maxVcal, iroc); 

    int itrim(0);
    fApi->_dut->updateTrimBits(ix, iy, 0, iroc);
    for (itrim = 0; itrim < 256; ++itrim) {
      fApi->setDAC("vtrim", itrim, iroc);
      vector<pixel> vpix = fApi->getThresholdMap("vcal", FLAG_RISING_EDGE, fParNtrig);
      int thr = 256;
      for (unsigned int ipx = 0; ipx < vpix.size(); ++ipx) {
	if (vpix[ipx].roc_id == iroc &&
	    vpix[ipx].column == ix &&
	    vpix[ipx].row == iy) {
	  thr = vpix[ipx].value;
	}
      }
      if (thr < fParVcal) {
	cout << "---------> " << thr << " < " << fParVcal << " for itrim = " << itrim << " ... break" << endl;
	break;
      }
    }
  
    fApi->setDAC("vtrim", itrim, iroc);
  }

  // -- set trim bits
  int correction = 4;
  setTrimBits();
  vector<TH1*> thr2  = thrMaps("vcal", "TrimThr2", fParNtrig); 
  vector<TH1*> thr2a = trimStep(correction, thr2);
  
  correction = 2; 
  vector<TH1*> thr3  = thrMaps("vcal", "TrimThr3", fParNtrig); 
  vector<TH1*> thr3a = trimStep(correction, thr3);
  
  correction = 1; 
  vector<TH1*> thr4  = thrMaps("vcal", "TrimThr4", fParNtrig); 
  vector<TH1*> thr4a = trimStep(correction, thr4);
  
  correction = 1; 
  vector<TH1*> thr5  = thrMaps("vcal", "TrimThr5", fParNtrig); 
  vector<TH1*> thr5a = trimStep(correction, thr5);
  
  PixTest::update(); 
}


// ----------------------------------------------------------------------
void PixTestTrim::trimBitTest() {

  LOG(logINFO) << "trimBitTest start "; 
  
  vector<int>vtrim; 
  vtrim.push_back(250);
  vtrim.push_back(200);
  vtrim.push_back(180);
  vtrim.push_back(140);

  vector<int>btrim; 
  btrim.push_back(14);
  btrim.push_back(13);
  btrim.push_back(11);
  btrim.push_back(7);

  vector<vector<TH1*> > steps; 

  ConfigParameters *cp = fPixSetup->getConfigParameters();
  // -- start untrimmed
  bool ok = cp->setTrimBits(15);
  if (!ok) {
    LOG(logWARNING) << "could not set trim bits to " << 15; 
    return;
  }
  fApi->setDAC("Vtrim", 0); 
  LOG(logDEBUG) << "trimBitTest determine threshold map without trims "; 
  vector<TH1*> thr0 = thrMaps("Vcal", "TrimBitsThr0", fParNtrig); 

  // -- now loop over all trim bits
  vector<TH1*> thr;
  for (unsigned int iv = 0; iv < vtrim.size(); ++iv) {
    thr.clear();
    ok = cp->setTrimBits(btrim[iv]);
    if (!ok) {
      LOG(logWARNING) << "could not set trim bits to " << btrim[iv]; 
      continue;
    }

    LOG(logDEBUG) << "trimBitTest initDUT with trim bits = " << btrim[iv]; 
    // FIXME: getRocPixelConfig SHOULD NOT RE-READ trim bit files and mask files...
    fApi->initDUT(cp->getTbmType(), cp->getTbmDacs(), 
		  cp->getRocType(), cp->getRocDacs(), 
		  cp->getRocPixelConfig());
    
    fApi->setDAC("Vtrim", vtrim[iv]); 
    LOG(logDEBUG) << "trimBitTest threshold map with trim = " << btrim[iv]; 
    thr = thrMaps("Vcal", Form("TrimThr_trim%d", btrim[iv]), fParNtrig); 
    steps.push_back(thr); 
  }

  LOG(logINFO) << "trimBitTest done "; 
  
}



// ----------------------------------------------------------------------
int PixTestTrim::adjustVtrim() {
  int vtrim = -1;
  int thr(255), thrOld(255);
  int ntrig(10); 
  do {
    vtrim++;
    fApi->setDAC("Vtrim", vtrim);
    thrOld = thr;
    thr = pixelThreshold("Vcal", ntrig, 0, 100); 
    LOG(logDEBUG) << vtrim << " thr " << thr;
  }
  while (((thr > fParVcal) || (thrOld > fParVcal) || (thr < 10)) && (vtrim < 200));
  vtrim += 5;
  fApi->setDAC("Vtrim", vtrim);
  LOG(logINFO) << "Vtrim set to " <<  vtrim;
  return vtrim;
}


// ----------------------------------------------------------------------
vector<TH1*> PixTestTrim::trimStep(int correction, vector<TH1*> calOld) {
  
  int trim(0); 
  int trimBitsOld[16][52][80];
  for (unsigned int i = 0; i < calOld.size(); ++i) {
    int idx = getIdFromIdx(i); 
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 52; ++iy) {
	trimBitsOld[idx][ix][iy] = fTrimBits[idx][ix][iy];
	if (calOld[i]->GetBinContent(i+1, iy+1) > fParVcal) {
	  trim = fTrimBits[idx][ix][iy] - correction; 
	} else {
	  trim = fTrimBits[idx][ix][iy] + correction; 
	}
	if (trim < 0) trim = 0; 
	if (trim > 15) trim = 15; 
	fTrimBits[idx][ix][iy] = trim; 
      }
    }
  } 	

  setTrimBits(); 
  vector<TH1*> calNew = thrMaps("vcal", Form("TrimThrCorr%d", correction), fParNtrig); 

  // -- check that things got better, else revert and leave up to next correction round
  for (unsigned int i = 0; i < calOld.size(); ++i) {
    int idx = getIdFromIdx(i); 
    for (int ix = 0; ix < 52; ++ix) {
      for (int iy = 0; iy < 52; ++iy) {
	if (TMath::Abs(calOld[idx]->GetBinContent(ix+1, iy+1) - fParVcal) < TMath::Abs(calNew[idx]->GetBinContent(ix+1, iy+1) - fParVcal)) {
	  trim = trimBitsOld[idx][ix][iy];
	  calNew[idx]->SetBinContent(ix+1, iy+1, calOld[idx]->GetBinContent(ix+1, iy+1)); 
	} else {
	  trim = fTrimBits[idx][ix][iy]; 
	}
	if (calNew[idx]->GetBinContent(ix+1, iy+1) > 0) 
	  cout << "roc/col/row: " << idx << "/" << ix << "/" << iy << " vcalThr = " << calNew[idx]->GetBinContent(ix+1, iy+1) << endl;
      }
    }
  } 	
  setTrimBits(); 

  return calNew;
}


// ----------------------------------------------------------------------
void PixTestTrim::setTrimBits() {
  vector<uint8_t> rocIds = fApi->_dut->getEnabledRocIDs(); 
  for (unsigned int iroc = 0; iroc < rocIds.size(); ++iroc){
    vector<pixelConfig> pix = fApi->_dut->getEnabledPixels(iroc);
    for (unsigned int ipix = 0; ipix < pix.size(); ++ipix) {
      fApi->_dut->updateTrimBits(pix[ipix].column, pix[ipix].row, fTrimBits[rocIds[iroc]][pix[ipix].column][pix[ipix].row], rocIds[iroc]);
    }
  }
}
