/*
    This file is part of Horst.

    Horst is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Horst is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Horst.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <TFile.h>
#include <TH1.h>

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>

#include "Config.h"
#include "InputFileReader.h"

using std::cin;
using std::cout;
using std::endl;
using std::ifstream;
using std::ofstream;
using std::string;
using std::stringstream;

void InputFileReader::readInputFile(const TString inputfilename, vector<TString> &filenames, vector<Double_t> &energies, vector<Double_t> &n_simulated_particles){
	
	cout << "> Reading input file " << inputfilename << " ..." << endl;

	ifstream file;	
	file.open(inputfilename);
	string line, energy, filename, n_particles;
	stringstream sst;

	if(file.is_open()){
		while(getline(file, line)){
			sst.str(line);
			sst >> filename >> energy >> n_particles;
			filenames.push_back(filename);
			energies.push_back(atof(energy.c_str()));
			n_simulated_particles.push_back(atof(n_particles.c_str()));
			sst.clear();
		}

	} else{
		cout << "Error: File " << inputfilename << " could not be opened." << endl;
	}
}

void InputFileReader::fillMatrix(const vector<TString> &filenames, const vector<Double_t> &energies, const vector<Double_t> &n_particles, const TString histname, TH2F &response_matrix, TH1F &n_simulated_particles){
	cout << "> Creating matrix ..." << endl;

	Double_t min_dist = (Double_t) NBINS;
	Double_t dist;
	Int_t best_simulation = 0;
	Int_t n_energies = (Int_t) energies.size();
	Int_t simulation_shift = 0;

	for(Int_t i = 1; i <= (Int_t) NBINS; ++i){
		// Find best simulation for energy bin
		min_dist = (Double_t) NBINS;
		best_simulation = 0;

		// Do not calculate the absolute value of dist immediately, because it will be
		// used later to shift the simulation in the right direction
		for(Int_t j = 0; j < n_energies; ++j){
			dist = energies[(long unsigned int) j] - (Double_t) i;
			if(fabs(dist) < fabs(min_dist)){
				min_dist = dist;
				best_simulation = j;
			}
		}

		cout << "Bin: " << i << " keV, using " << filenames[(long unsigned int) best_simulation] << " ( " << energies[(long unsigned int) best_simulation] << " )" << endl;

		// Fill row of matrix with best simulation
		TFile *inputFile = new TFile(filenames[(long unsigned int) best_simulation]);
		TH1F *hist = nullptr;

		if(gDirectory->FindKey(histname)){
			hist = (TH1F*) gDirectory->Get(histname);
		} else{
			cout << __FILE__ << ":" << __FUNCTION__ << "():" << __LINE__ << ": Error: No TH1F object called '" << histname << "' found in '" << filenames[(long unsigned int) best_simulation] << "'. Aborting ..." << endl; 
			abort();
		}

		simulation_shift = (Int_t) min_dist;	
		for(Int_t j = 1; j <= (Int_t) NBINS; ++j){
			if(j + simulation_shift < (Int_t) NBINS && (j + simulation_shift) >= 0){
				response_matrix.SetBinContent(i, j, hist->GetBinContent(j + simulation_shift));
			}
		}

		// Fill number of simulated particles into TH1F
		n_simulated_particles.SetBinContent(i, n_particles[(long unsigned int) best_simulation]);

		inputFile->Close();
	}
}

void InputFileReader::updateMatrix(const vector<TString> &old_filenames, const vector<Double_t> &old_energies, const vector<Double_t> &old_n_particles, const TH2F &old_response_matrix, const vector<TString> &new_filenames, const vector<Double_t> &new_energies, const vector<Double_t> &new_n_particles, const TString histname, TH2F &response_matrix, TH1F &n_simulated_particles){
	cout << "> Updating matrix ..." << endl;

	Double_t min_dist_old = (Double_t) NBINS;
	Double_t min_dist_new = (Double_t) NBINS;
	Double_t dist_old;
	Double_t dist_new;
	Int_t best_simulation_old = 0;
	Int_t best_simulation_new = 0;
	Int_t n_energies_old = (Int_t) old_energies.size();
	Int_t n_energies_new = (Int_t) new_energies.size();
	Int_t simulation_shift = 0;

	for(Int_t i = 1; i <= (Int_t) NBINS; ++i){
		// Find best simulation for energy bin
		min_dist_old = (Double_t) NBINS;
		min_dist_new = (Double_t) NBINS;
		best_simulation_old = 0;
		best_simulation_new = 0;

		for(Int_t j = 0; j < n_energies_old; ++j){
		// Do not calculate the absolute value of dist immediately, because it will be
		// used later to shift the simulation in the right direction
			dist_old = old_energies[(long unsigned int) j] - (Double_t) i;	
			if(fabs(dist_old) < fabs(min_dist_old)){
				min_dist_old = dist_old;
				best_simulation_old = j;
			}
		}

		for(Int_t j = 0; j < n_energies_new; ++j){
		// Do not calculate the absolute value of dist immediately, because it will be
		// used later to shift the simulation in the right direction
			dist_new = new_energies[(long unsigned int) j] - (Double_t) i;
			if(fabs(dist_new) < fabs(min_dist_new)){
				min_dist_new = dist_new;
				best_simulation_new = j;
			}
		}

		if(fabs(min_dist_new) < fabs(min_dist_old)){
			cout << "Bin: " << i << " keV, using new simulation " << new_filenames[(long unsigned int) best_simulation_new] << " ( " << new_energies[(long unsigned int) best_simulation_new] << " )" << endl;
			//
			// Fill row of matrix with best simulation
			TFile *inputFile = new TFile(new_filenames[(long unsigned int) best_simulation_new]);
			TH1F *hist = nullptr;

			if(gDirectory->FindKey(histname)){
				hist = (TH1F*) gDirectory->Get(histname);
			} else{
				cout << __FILE__ << ":" << __FUNCTION__ << "():" << __LINE__ << ": Error: No TH1F object called '" << histname << "' found in '" << new_filenames[(long unsigned int) best_simulation_new] << "'. Aborting ..." << endl; 
				abort();
			}

			simulation_shift = (Int_t) min_dist_new;	
			for(Int_t j = 1; j <= (Int_t) NBINS; ++j){
				if(j + simulation_shift < (Int_t) NBINS && (j + simulation_shift) >= 0){
					response_matrix.SetBinContent(i, j, hist->GetBinContent(j + simulation_shift));
				}
			}

			// Fill number of simulated particles into TH1F
			n_simulated_particles.SetBinContent(i, new_n_particles[(long unsigned int) best_simulation_new]);

			inputFile->Close();

		} else{
			cout << "Bin: " << i << " keV, keep old simulation ( " << old_energies[(long unsigned int) best_simulation_old] << " )" << endl;

			n_simulated_particles.SetBinContent(i, old_n_particles[best_simulation_old]);
			for(Int_t j = 1; j <= (Int_t) NBINS; ++j){
				if(j + simulation_shift < (Int_t) NBINS && (j + simulation_shift) >= 0){
					response_matrix.SetBinContent(i, j, old_response_matrix.GetBinContent(i, j));
				}
			}
		}
	}
}

void InputFileReader::writeMatrix(TH2F &response_matrix, TH1F &n_simulated_particles, TString outputfilename) const {
	TFile *outputFile = new TFile(outputfilename, "RECREATE");	

	response_matrix.Write();
	n_simulated_particles.Write();
	outputFile->Close();

	cout << "> Wrote matrix to file " << outputfilename << endl;
}

void InputFileReader::readMatrix(TH2F &response_matrix, TH1F &n_simulated_particles, const TString matrixfile){

	TFile *inputFile = new TFile(matrixfile); 
	TH2F *rema = nullptr;

	if(gDirectory->FindKey("rema")){
		rema = (TH2F*) gDirectory->Get("rema");
	} else{
		cout << __FILE__ << ":" << __FUNCTION__ << "():" << __LINE__ << ": Error: No TH2F object called 'rema' found in '" << matrixfile << "'. Aborting ..." << endl;
		abort();
	}

	for(Int_t i = 1; i <= (Int_t) NBINS; ++i){
		for(Int_t j = 1; j <= (Int_t) NBINS; ++j){
			response_matrix.SetBinContent(i, j, rema->GetBinContent(i, j));
		}
	}

	TH1F *n_particles = nullptr;
	
	if(gDirectory->FindKey("n_simulated_particles")){
		n_particles = (TH1F*) gDirectory->Get("n_simulated_particles");
	} else{
		cout << __FILE__ << ":" << __FUNCTION__ << "():" << __LINE__ << ": Error: No TH1F object called 'n_particles' found in '" << matrixfile << "'. Aborting ..." << endl;
		abort();
	}
	for(Int_t i = 1; i <= (Int_t) NBINS; ++i){
		n_simulated_particles.SetBinContent(i, n_particles->GetBinContent(i));
	}

	inputFile->Close();
}

void InputFileReader::readMatrix(TH2F &response_matrix, const TString matrixfile){

	TFile *inputFile = new TFile(matrixfile); 
	TH2F *rema = nullptr;

	if(gDirectory->FindKey("rema")){
		rema = (TH2F*) gDirectory->Get("rema");
	} else{
		cout << __FILE__ << ":" << __FUNCTION__ << "():" << __LINE__ << ": Error: No TH2F object called 'rema' found in '" << matrixfile << "'. Aborting ..." << endl;
		abort();
	}

	for(Int_t i = 1; i <= (Int_t) NBINS; ++i){
		for(Int_t j = 1; j <= (Int_t) NBINS; ++j){
			response_matrix.SetBinContent(i, j, rema->GetBinContent(i, j));
		}
	}

	inputFile->Close();
}

void InputFileReader::readTxtSpectrum(TH1F &spectrum, const TString spectrumfile){
	
	ifstream file;	
	file.open(spectrumfile);
	string line = "";
	Int_t index = 1;

	if(file.is_open()){
		while(getline(file, line)){
			spectrum.SetBinContent(index, atof(line.c_str()));
			++index;
		}
	}
}

void InputFileReader::readROOTSpectrum(TH1F &spectrum, const TString spectrumfile, const TString spectrumname){
	
	TFile file;
	file.Open(spectrumfile, "READ");

	TH1F *spec= (TH1F*) gDirectory->Get(spectrumname);

	for(Int_t i = 0; i <= (Int_t) NBINS; ++i){
		spectrum.SetBinContent(i, spec->GetBinContent(i));
	}
}

void InputFileReader::writeCorrelationMatrix(TMatrixDSym &correlation_matrix, TString outputfilename) const {
	ofstream outputfile;
	outputfile.open(outputfilename);

	Int_t n_rows = correlation_matrix.GetNrows();

	for(Int_t i = 0; i < n_rows; ++i){
		for(Int_t j = 0; j < n_rows; ++j){
			outputfile << correlation_matrix(i,j) << "\t";
		}
		outputfile << "\n";
	}

	outputfile.close();
}

void InputFileReader::readDoubleParameters(vector<Double_t> &params, const TString inputfilename){
	
	cout << "> Reading input file " << inputfilename << " ..." << endl;

	ifstream file;
	file.open(inputfilename);
	string line, parameter;
	stringstream sst;

	if(file.is_open()){
		getline(file, line);
		sst.str(line);
		while(sst >> parameter)
			params.push_back((UInt_t) atoi(parameter.c_str()));
		file.close();
	} else{
		cout << "Error: File " << inputfilename << " could not be opened." << endl;
	}
}

void InputFileReader::readUnsignedIntParameters(vector<UInt_t> &params, const TString inputfilename){
	
	cout << "> Reading input file " << inputfilename << " ..." << endl;

	ifstream file;
	file.open(inputfilename);
	string line, parameter;
	stringstream sst;

	if(file.is_open()){
		getline(file, line);
		sst.str(line);
		while(sst >> parameter)
			params.push_back((UInt_t) atoi(parameter.c_str()));
		file.close();
	} else{
		cout << "Error: File " << inputfilename << " could not be opened." << endl;
	}
}

void InputFileReader::writeParameters(const vector<UInt_t> &params, const TString inputfilename) const {

	cout << "Writing input file " << inputfilename << " ..." << endl;

	ofstream file;	
	file.open(inputfilename);

	if(file.is_open()){
		for(auto p : params)
			file << p << "\t";
		file.close();
	} else{
		cout << "Error: File " << inputfilename << " could not be opened." << endl;
	}
}

void InputFileReader::writeParameters(const vector<Double_t> &params, const TString inputfilename) const {

	cout << "Writing input file " << inputfilename << " ..." << endl;

	ofstream file;	
	file.open(inputfilename);

	if(file.is_open()){
		for(auto p : params)
			file << p << "\t";
		file.close();
	} else{
		cout << "Error: File " << inputfilename << " could not be opened." << endl;
	}
}
