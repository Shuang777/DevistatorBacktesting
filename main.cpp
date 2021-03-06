//	This is the main file that executes a strategy over historical data. 
//
//	To do:	[X] calc number of trades (how many times the pos vec changes sign)
//			[X] calculate Sharpe Ratio
//			[X] include transaction costs
//			[X] fix maxDD to span multiple days

#include <iostream>
#include <vector>
#include <dirent.h>
#include <time.h>
#include "Strategy.h"
#include "MomentumStrat3.h"
#include "histTextLoad.h"
#include "stratEval.h"

using namespace std;
const int NUM_HOLDOUT_ITER = 1;

int getDir (string dir, vector<string> &files);
stratResu stratEval(Strategy * stratUnderTest, vector<timeSlice> testData, maxDDStore* maxDDPass);
double calcStd(double mean, double dailyRets[], int numDays);
double calcStdDist(double totalReturns, double partStatSumSq, int numMins);


int main (int argc, char * const argv[]) 
{
	double sumReturns = 0;
	for (int j=0; j < NUM_HOLDOUT_ITER; j++)
	{
		
		vector <string> trainingSet = vector <string>();
		int numFiles = getDir(".", trainingSet);

		//split up the files into training and test sets
		vector <string> testSet = vector <string>();
		srand(time(NULL));  //seed the RNG
		int randn;
		for (int p=0; p < numFiles/10; p++)
		{
			randn = rand() % trainingSet.size();		      //get a random number randn [0, trainingSet.size()]
			testSet.push_back(trainingSet[randn]);	    	  //add element at index randn, to testSet
			trainingSet.erase(trainingSet.begin() + randn);   //remove element at index randn from trainingSet
		}

		//loop over training set
		MomentumStrat3 * myStrat;
		stratResu myStratResults;
		double retAfterTransCost, totalReturns = 0.0;
		double returnArr[numFiles], mean, standDev, sharpeRatio, timeInMktSum = 0.0;
		maxDDStore maxDDPass;
		initDDstore(&maxDDPass);
		int partStatN = 0.0;
		double partStatSumSq = 0.0;
		int posDays = 0; 
		int negDays = 0;
		int totalTrades = 0;
		int numTradingDays = trainingSet.size();
		for (int k=0; k < numTradingDays; k++)
		{
			//First step load data into vector
			vector<timeSlice> histData = histTextLoad(trainingSet[k]);
			if (histData.size() == 0) return 0;		//stop the program if no data was returned
			
			//Second step: sort vector by date (may not be needed depending on input file)
			sort(histData.begin(), histData.end(), timeSliceSortPred);
			
			//Third step: feed stream to strategy class 
			myStrat = new MomentumStrat3(atof(argv[1]), atof(argv[2]), atof(argv[3]));
			myStratResults = stratEval(myStrat, histData, &maxDDPass);

			partStatN += myStratResults.partStatN;
			partStatSumSq += myStratResults.partStatSumSq;
			
			totalReturns += myStratResults.totalRet;
			returnArr[k] = myStratResults.totalRet;
			if (myStratResults.totalRet >= 0) posDays++;
			else							  negDays++; 

			delete myStrat;

			totalTrades += myStratResults.numberTrades;
			timeInMktSum += myStratResults.percInMkt;
		}
		printf("*******$$$$$$$*******$$$$$$$*******\n");
		retAfterTransCost = totalReturns - (2.5/40000.0)*totalTrades;
		printf("totalReturns: %5f, returns after transaction costs: %f\n", totalReturns, retAfterTransCost);
		mean = totalReturns/numTradingDays;
		standDev = calcStd(mean, returnArr, numTradingDays);
		mean = retAfterTransCost/numTradingDays;
		//standDev = calcStdDist(totalReturns, partStatSumSq, partStatN);
		sharpeRatio = 15.8745 * mean / standDev;  //this is anualized Sharpe's ratio 15.875 = sqrt(252)
		printf("mean (daily) return is: %f, the return (daily) std is: %f\n", mean, standDev);
		printf("anualized Sharpe's ratio: %f\n", sharpeRatio);
		printf("percentage time in the mkt: %3f\n", timeInMktSum/numTradingDays);
		printf("max draw down: %5f, maxDD time (min): %d\n", maxDDPass.maxDrawDown, maxDDPass.maxDDtime);//this is only the latest
		printf("number of trades: %d\n", totalTrades);
		printf("total positive days: %d total negative days: %d\n", posDays, negDays);
		sumReturns += retAfterTransCost;
	}
	printf("after %d iterations the avg return is: %f\n",NUM_HOLDOUT_ITER, sumReturns/NUM_HOLDOUT_ITER);
	return 0;
}
double calcStdDist(double totalReturns, double partStatSumSq, int numMins){
	double mean, varSum;		//this calcs the minute std from partial sums
	mean = totalReturns/(double)numMins;
	varSum = partStatSumSq - 2 * mean * totalReturns + (double)numMins * mean * mean;
	return sqrt(varSum/numMins);
}

//function for calculating standard deviation
double calcStd(double mean, double dailyRets[], int numDays){
	double var; 
	double varSum = 0.0; 
	for (int i=0; i<numDays; i++) {
		var = dailyRets[i] - mean;
		varSum += var * var;
	}
	
	return sqrt(varSum/numDays);
}
// function for opeing directory
//filters out ., and ..
int getDir (string dir, vector<string> &files){
	DIR *dp;
	struct dirent *dirp;
	if ((dp = opendir(dir.c_str())) == NULL ){
		cout << "error opening directory: "<< dir <<endl;
		return 0;
	}
	
	while ((dirp = readdir(dp))!=NULL) {
		string fileStr = string(dirp->d_name);
		if ((strcmp(fileStr.c_str(),".") != 0) && (strcmp(fileStr.c_str(),"..") != 0))   files.push_back(fileStr);
	}
	closedir(dp);
	return files.size();  //return the number of files in the directory
}

