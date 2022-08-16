#include <iostream>
#include <string>
#include <TTree.h>
#include <TH1F.h>
#include <TFile.h>
#include <TF1.h>
#include "TROOT.h"
#include "TSystem.h"
#include "TKey.h"
#include "TCanvas.h"
#include "TLine.h"

using namespace::std;

class PulseFinder{
public:
    TH1F *histogram;

    PulseFinder(){};

    //creates a histogram of the timestamps; this histogram will show when a pulse reaches the detector
    void createHistogram(string filename, string saveTo){
        //open file
        unique_ptr<TFile> myFile(TFile::Open(filename.c_str()));
        //get any tree in the file
        TIter next(myFile -> GetListOfKeys());
        TKey *key;
        string treeName;

        while ((key = (TKey*)next())) {
            //continue;
            TClass *clsPtr = gROOT->GetClass(key->GetClassName());
            TString name = key->GetClassName();
            if (! name.Contains("TTree")) continue;
            // do something with the key
            treeName = key->GetName();
        }

        TTree *t = (TTree *) myFile->Get(treeName.c_str());

        //load tree
        auto entries = t -> GetEntries();
        UShort_t energy;
        ULong64_t timestamp;
        UShort_t channel;
        t->SetBranchAddress("Channel", &channel);
        t->SetBranchAddress("Energy", &energy);
        t->SetBranchAddress("Timestamp", &timestamp);
        t->GetEntry(1);
        ULong64_t t_i = timestamp;
        t->GetEntry(entries-1);
        ULong64_t t_f = timestamp;

        //fill histogram
        string name = "pulsefinding";
        histogram = new TH1F(name.c_str(),name.c_str(),100000,t_i,t_f);
        for(int i = 0; i < entries; i++){
            t->GetEntry(i);
            if(channel == 1) continue;
            if(energy > 0){
                histogram -> Fill(timestamp);
            }
        }
        TFile *file = new TFile(saveTo.c_str(), "UPDATE");
        file->cd();

        histogram -> Write();
        file -> Close();

        myFile -> Close();
    }

    void loadHistogram(string loadFrom){
        TFile *myFile = TFile::Open(loadFrom.c_str());
        //unique_ptr<TFile> myFile(TFile::Open("pulseHistogram.root"));
        histogram = (TH1F *) myFile->Get("pulsefinding");
    }

    //iterate over all bins, when there is a large rise, it's probably the pulse.
    vector<ULong64_t> findPulses(int prom, string saveTo){
        //retrieve histogram that was create from the createHistogram method
        TFile *myFile = TFile::Open(saveTo.c_str());
        histogram = (TH1F *) myFile->Get("pulsefinding;1");

        vector<ULong64_t> timeStamps = {};
        bool recentPulse = false;
        int ticksSinceLastPulse = 0;

        //loop over all bins in histogram. The idea here is that we do an average of the previous 30 bins. If the
        //average of the next 3 bins is larger by some threshold, we say its a pulse. This threshould is determined
        //by the binning of the timestamp histogram, which is dependent of how long the run was, and its dependent
        //on the rate on the detector. It is therefore a bit arbitrary what the number is, the user must choose one
        //that works.
        for(int i = 0; i < histogram -> GetNbinsX(); i++){
            //dont want to do trigger on a pulse immediately after i just found one.
            ticksSinceLastPulse++;
            if(ticksSinceLastPulse > 50){ recentPulse = false;}
            if(recentPulse){continue;}
            if(ticksSinceLastPulse < 50){continue;}

            //find the average rate of the 30 previous ticks
            double prev30avg = 0;
            int k = 0;
            if(i > 30){
                for(int j = 1; j < 31; j++){
                    prev30avg += histogram -> GetBinContent(i-j);
                    k++;
                }
            }
            prev30avg = prev30avg/k;

            //find the average rate of the next 3 ticks
            k = 0;
            double next3avg = 0;
            for(int j = 0; j < 3; j++){
                next3avg += histogram -> GetBinContent(i+j);
                k++;
            }
            next3avg = next3avg/k;

            //if the average of next 3 is higher by "prom", we say its a pulse.
            if(next3avg > prev30avg + prom){
                recentPulse = true;
                ticksSinceLastPulse = 0;
                timeStamps.push_back(histogram ->GetBinCenter(i));
            }
        }

        //draw the pulses on to the histogram for the user to see.
        TCanvas *c1= new TCanvas;
        histogram -> Draw();
        for(int i = 0; i < timeStamps.size(); i++){
            TLine *l=new TLine(timeStamps[i],0,timeStamps[i],histogram->GetMaximum());
            l->SetLineColor(kBlack);
            l->Draw();
        }
        TFile *file = new TFile(saveTo.c_str(), "UPDATE");
        file->cd();
        c1 -> Write();
        file -> Close();
        myFile -> Close();

        return timeStamps;
    }
};

int main(int argc, char *argv[]) {
    //get input from user
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
    int prom;

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

    input_wrong = true;
    while(input_wrong){
        string lower;
        cout << "Enter peak prominence (arbitrary number determining how much rate should rise: default is 10. \n If too few pulses are detected, lower this; if too many pulses are detected, raise the number): "; // Type a number and press enter
        getline(std::cin, lower); // Get user input from the keyboard

        if(lower.size() == 0){
            prom = 10;
            input_wrong = false;
        }
        else{
            prom = stoi(lower);
            input_wrong = false;
        }
    }

    //open file
    unique_ptr<TFile> myFile(TFile::Open(filename.c_str()));

    //get any tree
    TIter next(myFile -> GetListOfKeys());
    TKey *key;
    string treeName;

    while ((key = (TKey*)next())) {

        TClass *clsPtr = gROOT->GetClass(key->GetClassName());
        TString name = key->GetClassName();
        if (! name.Contains("TTree")) continue;
        // do something with the key
        treeName = key->GetName();
    }

    //load tree
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
    histFile -> Close();

    //create histograms
    string earlyHistName = "early" + to_string(t1_lower) + "ms to " + to_string(t2_lower)+ "ms 1keV";
    string laterHistName = "late" +to_string(t1_higher) + "ms to " + to_string(t2_higher)+ "ms 1keV";
    string earlyHistName5keV = "early" +to_string(t1_lower) + "ms to " + to_string(t2_lower) + "ms 0.5keV";
    string laterHistName5keV = "late" + to_string(t1_higher) + "ms to " + to_string(t2_higher)+ "ms 0.5keV";
    string earlyHistName25keV = "early" +to_string(t1_lower) + "ms to " + to_string(t2_lower) + "ms 0.25keV";
    string laterHistName25keV = "late" +to_string(t1_higher) + "ms to " + to_string(t2_higher)+ "ms 0.25keV";

    auto earlyHist = new TH1F(earlyHistName.c_str(),earlyHistName.c_str(),5000,0,5000);
    auto laterHist = new TH1F(laterHistName.c_str(),laterHistName.c_str(),5000,0,5000);
    auto dHist = new TH1F("dHist 1keV","dHist 1keV",5000,0,5000);
    auto earlyHist5keV = new TH1F(earlyHistName5keV.c_str(),earlyHistName5keV.c_str(),10000,0,5000);
    auto laterHist5keV = new TH1F(laterHistName5keV.c_str(),laterHistName5keV.c_str(),10000,0,5000);
    auto dHist5keV = new TH1F("dHist 0.5keV","dHist 0.5keV",10000,0,5000);
    auto earlyHist25keV = new TH1F(earlyHistName25keV.c_str(),earlyHistName25keV.c_str(),20000,0,5000);
    auto laterHist25keV = new TH1F(laterHistName25keV.c_str(),laterHistName25keV.c_str(),20000,0,5000);
    auto dHist25keV = new TH1F("dHist 0.25keV","dHist 0.25keV",20000,0,5000);

    //first find the pulses in the histogram. This is done by the pulseFinder class
    auto puls = new PulseFinder();
    puls ->createHistogram(filename, histfilename);
    auto timeStamps = puls ->findPulses(prom, histfilename);
    int currentTimeStampIndex = 0;

    ULong64_t timestampSorted;

    //iterate over tree
    for(int i = 0; i < entries; i++){
        if(channel == 1) continue;
        //dont want to start in middle of collection: start at first pulse
        if(timestamp < timeStamps[0]) continue;
        if(currentTimeStampIndex < timeStamps.size()-1){
            if(timestamp > timeStamps[currentTimeStampIndex+1]){currentTimeStampIndex++;}
        }
        //get timestamp with reference to start of pulse
        timestampSorted = timestamp - timeStamps[currentTimeStampIndex];

        if(timestampSorted > t1_lower*1e9 && timestampSorted < t2_lower*1e9){
            earlyHist ->Fill(energy);
            earlyHist5keV ->Fill(energy);
            earlyHist25keV ->Fill(energy);
        }

        if(timestampSorted > t1_higher*1e9 && timestampSorted < t2_higher*1e9){
            laterHist ->Fill(energy);
            laterHist5keV ->Fill(energy);
            laterHist25keV ->Fill(energy);
        }
    }

    histFile = TFile::Open(histfilename.c_str(), "UPDATE");
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

    TCanvas *c1 = new TCanvas("Both 1keV","Both 1keV");
    c1 -> cd();
    earlyHist->SetLineColor(kRed);
    earlyHist->SetAxisColor(kRed);
    earlyHist->SetMarkerColor(kRed);
    earlyHist->Draw();
    laterHist->Draw("SAME");

    TCanvas *c2 = new TCanvas("Both 0.5keV","Both 0.5keV");
    c2 -> cd();
    earlyHist5keV->SetLineColor(kRed);
    earlyHist5keV->SetAxisColor(kRed);
    earlyHist5keV->SetMarkerColor(kRed);
    earlyHist5keV->Draw();
    laterHist5keV->Draw("SAME");

    TCanvas *c3 = new TCanvas("Both 0.25keV","Both 0.25keV");
    c3 -> cd();
    earlyHist25keV->SetLineColor(kRed);
    earlyHist25keV->SetAxisColor(kRed);
    earlyHist25keV->SetMarkerColor(kRed);
    earlyHist25keV->Draw();
    laterHist25keV->Draw("SAME");

    histFile -> cd();
    c1 -> Write();
    c2 -> Write();
    c3 -> Write();

    histFile -> Close();
    myFile -> Close();

    cout << "Histograms written to " << histfilename << endl;
}
