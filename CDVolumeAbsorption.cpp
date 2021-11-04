#include "sierrachart.h"

SCDLLName("CD-Volume Absorption")

bool doReset(SCStudyInterfaceRef sc, int idx) {
	if (idx == 0)
		return true;

	SCDateTime prevStartOfPeriod = sc.GetStartOfPeriodForDateTime(sc.BaseDateTimeIn[idx - 1], TIME_PERIOD_LENGTH_UNIT_DAYS, 1, 0, 0);
	SCDateTime startOfPeriod = sc.GetStartOfPeriodForDateTime(sc.BaseDateTimeIn[idx], TIME_PERIOD_LENGTH_UNIT_DAYS, 1, 0, 0);

	return startOfPeriod != prevStartOfPeriod;
}

void normalize(SCSubgraphRef data, int startIdx, int length, float rangeMin, float rangeMax) {
	float h = rangeMax - rangeMin;
	if (h == 0) {
		for (int i = startIdx; i < startIdx + length; i++) {
			data[i] = 0;
			data.Arrays[0][i] = 0;
		}
	}
	else {
		for (int i = startIdx; i < startIdx + length; i++) {
			data[i] = (data[i] - rangeMin) / h;
			data.Arrays[0][i] = (data.Arrays[0][i] - rangeMin) / h;
		}
	}
}

void initialize(SCStudyInterfaceRef sc) {
	SCSubgraphRef PriceVol = sc.Subgraph[0];
	SCSubgraphRef _CDVol = sc.Subgraph[1];
	SCSubgraphRef _CumPrice = sc.Subgraph[2];

	for (int i = 0; i < sc.ArraySize; i++)
	{
		bool isReset = doReset(sc, i);
		sc.CumulativeDeltaVolume(sc.BaseDataIn, _CDVol, i, isReset);
	}

	float minPrice = FLT_MAX;
	float maxPrice = FLT_MIN;
	float minVolCD = FLT_MAX;
	float maxVolCD = FLT_MIN;
	int fromIdx = 0;
	for (int i = 0; i < sc.ArraySize; i++)
	{
		if (doReset(sc, i) && i != 0) {
			normalize(_CumPrice, fromIdx, i-fromIdx, minPrice, maxPrice);
			normalize(_CDVol, fromIdx, i-fromIdx, minVolCD, maxVolCD);
			fromIdx = i;
			minPrice = FLT_MAX;
			maxPrice = FLT_MIN;
			minVolCD = FLT_MAX;
			maxVolCD = FLT_MIN;
		}
		_CumPrice[i] = sc.Close[i];
		_CumPrice.Arrays[0][i] = sc.Open[i];
		minPrice = min(sc.Low[i], minPrice);
		maxPrice = max(sc.High[i], maxPrice);
		minVolCD = min(_CDVol.Arrays[2][i], minVolCD);
		maxVolCD = max(_CDVol.Arrays[1][i], maxVolCD);
	}
	normalize(_CumPrice, fromIdx, sc.ArraySize-fromIdx, minPrice, maxPrice);
	normalize(_CDVol, fromIdx, sc.ArraySize-fromIdx, minVolCD, maxVolCD);
	
	float dPriceVol = 0.0;
	for (int i = 0; i < sc.ArraySize; i++)
	{
		float dp = _CumPrice[i] - _CumPrice.Arrays[0][i];
		float dv = _CDVol[i] - _CDVol.Arrays[0][i];
		if (doReset(sc, i) && i != 0) {
			dPriceVol = 0.0;
		}
		dPriceVol += dv - dp;
		PriceVol[i] = dPriceVol;
	}
}

void update(SCStudyInterfaceRef sc) {
	SCSubgraphRef PriceVol = sc.Subgraph[0];
	SCSubgraphRef _CDVol = sc.Subgraph[1];
	SCSubgraphRef _CumPrice = sc.Subgraph[2];

	SCDateTime StartDateTime = sc.GetTradingDayStartDateTimeOfBar(sc.BaseDateTimeIn[sc.UpdateStartIndex]);
	int startIdx = sc.GetContainingIndexForSCDateTime(sc.ChartNumber, StartDateTime);

	for (int i = startIdx; i < sc.ArraySize; i++)
	{
		bool isReset = doReset(sc, i);
		sc.CumulativeDeltaVolume(sc.BaseDataIn, _CDVol, i, isReset);
	}

	float minPrice = FLT_MAX;
	float maxPrice = FLT_MIN;
	float minVolCD = FLT_MAX;
	float maxVolCD = FLT_MIN;
	
	for (int i = startIdx; i < sc.ArraySize; i++)
	{
		_CumPrice[i] = sc.Close[i];
		_CumPrice.Arrays[0][i] = sc.Open[i];
		minPrice = min(sc.Low[i], minPrice);
		maxPrice = max(sc.High[i], maxPrice);
		minVolCD = min(_CDVol.Arrays[2][i], minVolCD);
		maxVolCD = max(_CDVol.Arrays[1][i], maxVolCD);
	}
	normalize(_CumPrice, startIdx, sc.ArraySize-startIdx, minPrice, maxPrice);
	normalize(_CDVol, startIdx, sc.ArraySize-startIdx, minVolCD, maxVolCD);
	
	float dPriceVol = 0.0;
	for (int i = startIdx; i < sc.ArraySize; i++)
	{
		float dp = _CumPrice[i] - _CumPrice.Arrays[0][i];
		float dv = _CDVol[i] - _CDVol.Arrays[0][i];
		dPriceVol += dv - dp;
		PriceVol[i] = dPriceVol;
	}
}

SCSFExport scsf_Absorption(SCStudyInterfaceRef sc)
{
	SCSubgraphRef PriceVol = sc.Subgraph[0];
	SCSubgraphRef _CDVol = sc.Subgraph[1];
	SCSubgraphRef _CumPrice = sc.Subgraph[2];

	if (sc.SetDefaults)
	{
		sc.GraphName = "CD-Volume Absorption";

		sc.AutoLoop = 0;
		
		PriceVol.Name = "Absorption";
		PriceVol.DrawStyle = DRAWSTYLE_FILL_TO_ZERO;
		PriceVol.PrimaryColor = RGB(192, 192, 192);

		_CDVol.DrawStyle = DRAWSTYLE_IGNORE;
		_CumPrice.DrawStyle = DRAWSTYLE_IGNORE;
		
		return;
	}

	if (sc.UpdateStartIndex == 0)
		initialize(sc);
	else
		update(sc);
}
