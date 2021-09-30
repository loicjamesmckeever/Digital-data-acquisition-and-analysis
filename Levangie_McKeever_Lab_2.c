#include <formatio.h>
#include "toolbox.h"
#include <analysis.h>
#include <NIDAQmx.h>
#include <userint.h>
#include "Levangie_McKeever_Lab_2.h"


static int panelHandle;
static TaskHandle *taskHandle;
static float rate=10;
static float interval=10;
static char filename[100]="default";
static float64 array[10000];
static double fourierArray[8192];
static double powerArray[8192];
static double freqInterval;
static float frequency[8192];
static float dataArray[10];
static char filenameA[128]="Signal_";
static char filenameB[128]="Fourier_";


//Display UI panel
int main (int argc, char *argv[])
{
	if (InitCVIRTE (0, argv, 0) == 0)
		return -1;	/* out of memory */
	if ((panelHandle = LoadPanel (0, "Levangie_McKeever_Lab_2.uir", PANEL)) < 0)
		return -1;
	DisplayPanel (panelHandle);
	RunUserInterface ();
	DiscardPanel (panelHandle);
	return 0;
}

//collect and plot data
int CVICALLBACK acquireData (int panel, int control, int event,
							 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			// Clear previous acquisition
			DeleteGraphPlot (panelHandle, PANEL_GRAPH, -1, VAL_DELAYED_DRAW);
			DeleteGraphPlot (panelHandle, PANEL_GRAPH_2, -1, VAL_DELAYED_DRAW);
			
			int size=rate*interval; //Set array size
			DAQmxCreateTask ("Acquire Data", &taskHandle);
			//set differential mode
			DAQmxCreateAIVoltageChan (taskHandle, "Dev1/ai0", "Signal", DAQmx_Val_Diff, -10.0, 10.0, DAQmx_Val_Volts, "");
			DAQmxCfgSampClkTiming (taskHandle, "", rate, DAQmx_Val_Rising, DAQmx_Val_FiniteSamps, size); //set acquisition rate
			DAQmxStartTask (taskHandle); //start task
			DAQmxReadAnalogF64 (taskHandle, DAQmx_Val_Auto, -1, DAQmx_Val_GroupByChannel, array, size, &size, 0); //collect data
			DAQmxClearTask (taskHandle);
			
			//Plot Original Data
			float timeArray[size];
			float intervalStep=1/rate;
			float dataArray[size];
			for(int i=0; i<size; i=i+1)
				timeArray[i]= intervalStep*i;
			for(int j=0; j<size; j=j+1)
				dataArray[j]=array[j];
			PlotXY (panelHandle, PANEL_GRAPH_2, timeArray, dataArray, size, VAL_FLOAT, VAL_FLOAT, VAL_THIN_LINE,
               VAL_EMPTY_SQUARE, VAL_SOLID, 1, VAL_RED);
			SetPlotAttribute (panelHandle, PANEL_GRAPH_2, 1, ATTR_PLOT_LG_TEXT, "Signal");
			//Plot Fourier Transform
			
			double doubleInterval=interval;
			ConvertArrayType (dataArray, VAL_FLOAT, fourierArray, VAL_DOUBLE, size);
			AutoPowerSpectrum (fourierArray, POWER_2_13, doubleInterval, powerArray, &freqInterval);
		
			freqInterval = freqInterval*size;
			for(int i=0;i<8192;i=i+1)
				   frequency[i]=freqInterval*i;
			PlotXY (panelHandle, PANEL_GRAPH, frequency, powerArray, 4096, VAL_FLOAT, VAL_DOUBLE, VAL_THIN_LINE,
               VAL_EMPTY_SQUARE, VAL_SOLID, 1, VAL_RED);
			SetPlotAttribute (panelHandle, PANEL_GRAPH, 1, ATTR_PLOT_LG_TEXT, "FFT of signal");
			//Determine frequency based on the position of the highest peak
			float maximum = powerArray[0];
 			int location;
  			for (int c = 0; c < 8192; c++)
  			{
    			if (powerArray[c] > maximum)
    			{
       			maximum  = powerArray[c];
       			location = c+1;
    			}
  			}
			float c_y=maximum;
			float c_x=frequency[location];
			SetCtrlVal (panelHandle, PANEL_NUMERIC_3, c_x);
			PlotLine (panelHandle, PANEL_GRAPH, c_x, 0, c_x, c_y, VAL_BLUE);
			SetPlotAttribute (panelHandle, PANEL_GRAPH, 2, ATTR_PLOT_LG_TEXT, "Max peak position");
			break;
	}
	return 0;
}

int CVICALLBACK timeInterval (int panel, int control, int event,
							  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, PANEL_NUMERIC, &interval);
			break;
	}
	return 0;
}

int CVICALLBACK acquisitionRate (int panel, int control, int event,
								 void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			GetCtrlVal (panelHandle, PANEL_NUMERIC_2, &rate);
			break;
	}
	return 0;
}

int CVICALLBACK quit (int panel, int control, int event,
					  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			QuitUserInterface (0);
			break;
	}
	return 0;
}

int CVICALLBACK saveFile (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			strcat(filenameA,filename);
			strcat(filenameB,filename);
			
			//save signal data
			int size=rate*interval;
			float timeArray[size];
			float intervalStep=1/rate;
			float dataArray[size];
			for(int i=0; i<size; i=i+1)
				timeArray[i]= intervalStep*i;
			for(int j=0; j<size; j=j+1)
				dataArray[j]=array[j];
			float SaveFileA[size][2];  
			int j=0;
			   for (int i=0;i<size;i=i+1) {
				   SaveFileA[j][1]= dataArray[i];
			   	   j=j+1;
			   }
			   j=0;
			   for (int i=1; i<size;i=i+1) {
				   SaveFileA[j][0]=timeArray[i];
			   	   j=j+1;
			   }
			   int ssize=2*size;
			   ArrayToFile (&filenameA, SaveFileA, VAL_FLOAT, ssize, 2, VAL_DATA_MULTIPLEXED, VAL_GROUPS_AS_COLUMNS, VAL_SEP_BY_TAB, 10, VAL_ASCII, VAL_TRUNCATE);
			   
			   //save fourier data
			   float SaveFileB[8192][2];
			   j=0;
			   for (int i=0;i<8192;i=i+1) {
				   SaveFileB[j][1]= powerArray[i];
			   	   j=j+1;
			   }
			   j=0;
			   for (int i=1; i<8192;i=i+1) {
				   SaveFileB[j][0]=frequency[i];
			   	   j=j+1;
			   }
			   ArrayToFile (&filenameB, SaveFileB, VAL_FLOAT, 16384
							, 2, VAL_DATA_MULTIPLEXED, VAL_GROUPS_AS_COLUMNS, VAL_SEP_BY_TAB, 10, VAL_ASCII, VAL_TRUNCATE); 
			break;

			break;
	}
	return 0;
}

int CVICALLBACK fileName (int panel, int control, int event,
						  void *callbackData, int eventData1, int eventData2)
{
	switch (event)
	{
		case EVENT_COMMIT:
			  GetCtrlVal (panelHandle, PANEL_STRING, filename);
			break;
	}
	return 0;
}
