
#include "stdafx.h"	//yds

#include "ppsvm.h"

#include <iostream>
#include <string>
using namespace std;

int main_peng()	//yds, change name
{
    ppsvmTrain("file/data.txt", 2000);
    return 0;
//    ppsvmPredict();
}

