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
#include <fstream>

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

    double gammaLower;
    double gammaHigher;
    double backgroundLower1;
    double backgroundHigher1;
    double backgroundLower2;
    double backgroundHigher2;
    int prom;

    bool input_wrong = true;
    while(input_wrong){
        string lower;
        cout << "Enter energy range for gamma to be counted [lower, higher] in keV: "; // Type a number and press enter
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
            gammaLower = stof(lower.substr(0,commaPlace));
            gammaHigher = stof(lower.substr(commaPlace+1));
        }
        else{
            cout << "Error: you must enter two numbers seperated by a comma" << endl;
        }
    }

    input_wrong = true;
    while(input_wrong){
        string lower;
        cout << "Enter lower background energy range to be counted [lower, higher] in keV: "; // Type a number and press enter
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
            backgroundLower1 = stof(lower.substr(0,commaPlace));
            backgroundHigher1 = stof(lower.substr(commaPlace+1));
        }
        else{
            cout << "Error: you must enter two numbers seperated by a comma" << endl;
        }
    }

    input_wrong = true;
    while(input_wrong){
        string lower;
        cout << "Enter lower background energy range to be counted [lower, higher] in keV: "; // Type a number and press enter
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
            backgroundLower2 = stof(lower.substr(0,commaPlace));
            backgroundHigher2 = stof(lower.substr(commaPlace+1));
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
    string histfilename = filename.substr(0,filename.size()-5) + "pulseFinderOutput.root";
    TFile *histFile = TFile::Open(histfilename.c_str(), "RECREATE");
    histFile -> Close();

    //first find the pulses in the histogram. This is done by the pulseFinder class
    auto puls = new PulseFinder();
    puls ->createHistogram(filename, histfilename);
    auto timeStamps = puls ->findPulses(prom, histfilename);
    int currentTimeStampIndex = 0;

    ULong64_t timestampSorted;

    //create txt file for data to be written to
    string saveto = filename + "releasedata.txt";
    ofstream mytxt (saveto);

    long lowerBGnumber = 0;
    long higherBGnumber = 0;
    long gammaNumber = 0;

    double gammaWidth = gammaHigher - gammaLower;
    double backgroundWidth = (backgroundHigher1-backgroundLower1) + (backgroundHigher1-backgroundLower1);

    double weightedBackground = 0;

    mytxt << "PulseNo. \t No. of gammas \n";

    //iterate over tree
    for(int i = 0; i < entries; i++){
        if(channel == 1) continue;
        //dont want to start in middle of collection: start at first pulse
        if(timestamp < timeStamps[0]) continue;
        if(currentTimeStampIndex < timeStamps.size()-1){
            if(timestamp > timeStamps[currentTimeStampIndex+1]){
                //next pulse is here. Write data and reset
                weightedBackground = gammaWidth/backgroundWidth * (lowerBGnumber + higherBGnumber);
                mytxt << currentTimeStampIndex << "\t" << gammaNumber - weightedBackground << "\n";
                lowerBGnumber = 0;
                higherBGnumber =0;
                gammaNumber = 0;
                currentTimeStampIndex++;
            }

            if(energy >= gammaLower && energy <= gammaHigher) gammaNumber++;
            if(energy >= backgroundLower1 && energy <= backgroundHigher1) lowerBGnumber++;
            if(energy >= backgroundLower2 && energy <= backgroundHigher2) lowerBGnumber++;

        }
    }
    mytxt.close();
    cout << "Data written to " << saveto << endl;
}
