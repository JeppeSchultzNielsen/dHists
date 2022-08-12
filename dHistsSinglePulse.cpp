#include <iostream>
#include <string>
#include <TTree.h>
#include <TH1F.h>
#include <TFile.h>
#include <TF1.h>
#include "TROOT.h"
#include "TSystem.h"
#include "TKey.h"

using namespace::std;

int main(int argc, char *argv[]) {
    if(argc == 1){
        cout << "Error: you must enter the name of a root file" << endl;
        return 1;
    }
    if(argc > 2){
        cout << "Error: too many arguments" << endl;
        return 1;
    }
    string filename = argv[1];

    double t1_lower;
    double t2_lower;
    double t1_higher;
    double t2_higher;

    bool input_wrong = true;
    while(input_wrong){
        string lower;
        cout << "Enter lower timegate (ms) [t1_lower, t2_lower]: "; // Type a number and press enter
        getline(std::cin, lower); // Get user input from the keyboard

        int commaCount = 0;
        int commaPlace;
        for(int i = 0; i < lower.size(); i++){
            if(lower[i] == *","){
                commaCount++;
                commaPlace = i;
            }
        }
        if(commaCount == 1){
            input_wrong = false;
            t1_lower = stof(lower.substr(0,commaPlace));
            t2_lower = stof(lower.substr(commaPlace+1));
        }
        else{
            cout << "Error: you must enter two numbers seperated by a comma" << endl;
        }
    }

    input_wrong = true;
    while(input_wrong){
        string lower;
        cout << "Enter higher timegate (ms) [t1_higher, t2_higher]: "; // Type a number and press enter
        getline(std::cin, lower); // Get user input from the keyboard

        int commaCount = 0;
        int commaPlace;
        for(int i = 0; i < lower.size(); i++){
            if(lower[i] == *","){
                commaCount++;
                commaPlace = i;
            }
        }
        if(commaCount == 1){
            input_wrong = false;
            t1_higher = stof(lower.substr(0,commaPlace));
            t2_higher = stof(lower.substr(commaPlace+1));
        }
        else{
            cout << "Error: you must enter two numbers seperated by a comma" << endl;
        }
    }

    unique_ptr<TFile> myFile(TFile::Open(filename.c_str()));

    TIter next(myFile -> GetListOfKeys());
    TKey *key;
    string treeName;
    //TCanvas c1;
    while ((key = (TKey*)next())) {
        //continue;
        TClass *clsPtr = gROOT->GetClass(key->GetClassName());
        TString name = key->GetClassName();
        if (! name.Contains("TTree")) continue;
        // do something with the key
        treeName = key->GetName();
    }

    TTree *t = (TTree *) myFile->Get(treeName.c_str());
    auto entries = t -> GetEntries();
    Double_t energy;
    ULong64_t timestamp;
    UShort_t channel;
    t->SetBranchAddress("Channel", &channel);
    t->SetBranchAddress("CalibEnergy", &energy);
    t->SetBranchAddress("Timestamp", &timestamp);
    string histfilename = filename.substr(0,filename.size()-5) + "dHistsOutput.root";
    TFile *histFile = TFile::Open(histfilename.c_str(), "RECREATE");

    string earlyHistName = to_string(t1_lower) + " to " + to_string(t2_lower)+ " 1keV";
    string laterHistName = to_string(t1_higher) + " to " + to_string(t2_higher)+ " 1keV";
    string earlyHistName5keV = to_string(t1_lower) + " to " + to_string(t2_lower) + " 0.5keV";
    string laterHistName5keV = to_string(t1_higher) + " to " + to_string(t2_higher)+ " 0.5keV";
    string earlyHistName25keV = to_string(t1_lower) + " to " + to_string(t2_lower) + " 0.25keV";
    string laterHistName25keV = to_string(t1_higher) + " to " + to_string(t2_higher)+ " 0.25keV";

    auto earlyHist = new TH1F(earlyHistName.c_str(),earlyHistName.c_str(),5000,0,5000);
    auto laterHist = new TH1F(laterHistName.c_str(),laterHistName.c_str(),5000,0,5000);
    auto dHist = new TH1F("dHist 1keV","dHist 1keV",5000,0,5000);
    auto earlyHist5keV = new TH1F(earlyHistName5keV.c_str(),earlyHistName5keV.c_str(),10000,0,5000);
    auto laterHist5keV = new TH1F(laterHistName5keV.c_str(),laterHistName5keV.c_str(),10000,0,5000);
    auto dHist5keV = new TH1F("dHist 0.5keV","dHist 0.5keV",10000,0,5000);
    auto earlyHist25keV = new TH1F(earlyHistName25keV.c_str(),earlyHistName25keV.c_str(),20000,0,5000);
    auto laterHist25keV = new TH1F(laterHistName25keV.c_str(),laterHistName25keV.c_str(),20000,0,5000);
    auto dHist25keV = new TH1F("dHist 0.25keV","dHist 0.25keV",20000,0,5000);
    for(int i = 1; i < entries; i++){
        t ->GetEntry(i);
        if(timestamp > t1_lower*1e9 && timestamp < t2_lower*1e9){
            earlyHist ->Fill(energy);
            earlyHist5keV ->Fill(energy);
            earlyHist25keV ->Fill(energy);
        }

        if(timestamp > t1_higher*1e9 && timestamp < t2_higher*1e9){
            laterHist ->Fill(energy);
            laterHist5keV ->Fill(energy);
            laterHist25keV ->Fill(energy);
        }
    }

    dHist -> Add(earlyHist);
    dHist -> Add(laterHist,-1. * (t1_higher-t1_lower)/(t2_higher-t2_lower));
    dHist5keV -> Add(earlyHist5keV);
    dHist5keV -> Add(laterHist5keV,-1. * (t1_higher-t1_lower)/(t2_higher-t2_lower));
    dHist25keV -> Add(earlyHist25keV);
    dHist25keV -> Add(laterHist25keV,-1. * (t1_higher-t1_lower)/(t2_higher-t2_lower));

    histFile -> cd();
    earlyHist -> Write();
    laterHist -> Write();
    dHist -> Write();
    earlyHist5keV -> Write();
    laterHist5keV -> Write();
    dHist5keV -> Write();
    earlyHist25keV -> Write();
    laterHist25keV -> Write();
    dHist25keV -> Write();

    histFile -> Close();
    myFile -> Close();

    cout << "Histograms written to " << histfilename << endl;
}
