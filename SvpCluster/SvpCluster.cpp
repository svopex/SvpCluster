// The top of every source code file must include this line
#include "sierrachart.h"


/*****************************************************************************

For reference, please refer to the Advanced Custom Study Interface
and Language documentation on the Sierra Chart website.

*****************************************************************************/


// This line is required. Change the text within the quote
// marks to what you want to name your group of custom studies. 
SCDLLName("SvpCluster")


//This is the basic framework of a study function.
/*
Mark Volatile Bar Function
*/
SCSFExport scsf_MarkVolBarFunction(SCStudyGraphRef sc)
{
  SCSubgraphRef askMaxClusterPrice = sc.Subgraph[0];
  SCSubgraphRef askMinClusterPrice = sc.Subgraph[1];
  SCSubgraphRef bidMaxClusterPrice = sc.Subgraph[2];
  SCSubgraphRef bidMinClusterPrice = sc.Subgraph[3];

  SCInputRef minimumSizeOfCluster = sc.Input[0];
  SCInputRef inputMinimumVolume = sc.Input[1];
  SCInputRef askBidCompareMultiplicatorInputRef = sc.Input[2];
  SCInputRef maximumCountOfSmallVolumesInCluster = sc.Input[3];
  SCInputRef showClusterInAppropriatePartOfCandle = sc.Input[4];
  SCInputRef compareVolumeWithPreviousCandle = sc.Input[5];
  SCInputRef inputMinimumImbalanceVolume = sc.Input[6];
  SCInputRef askBidCompareLocalMultiplicatorInputRef = sc.Input[7];
  SCInputRef inputMinimumVolumeOnFirstLevel = sc.Input[8];

  if (sc.SetDefaults)
  {
	// During development set this flag to 1, so the DLL can be modified. When development is done, set it to 0 to improve performance.
	sc.FreeDLL = 1;

	sc.GraphName = "SvpCluster";
	sc.StudyDescription = "SvpCluster.";
	sc.AutoLoop = 1;
	sc.GraphRegion = 0;
	sc.ScaleRangeType = SCALE_SAMEASREGION;
	sc.MaintainVolumeAtPriceData = 1;
	sc.UpdateAlways = 1;

	// Smartegy nastaveni 
	// 
	// Minimum size of cluster: 3
	// Minimum volume: 0
	// Ask vs bid for the whole cluster: 1
	// Maximum count of small volumes in cluster: 0
	// Show cluster in appropriate part of candle: yes
	// Compare volume with previous candle: no
	// Minimum imbalance volume: 10
	// Ask vs bid on each row: 2

	minimumSizeOfCluster.SetInt(3);
	minimumSizeOfCluster.Name = "Minimum size of cluster"; // Minimalni pocet urovni v clusteru.
	inputMinimumVolume.SetInt(40);
	inputMinimumVolume.Name = "Minimum volume"; // Minimalni objem kdekoli v clusteru.
	askBidCompareMultiplicatorInputRef.SetDouble(1.1);
	askBidCompareMultiplicatorInputRef.Name = "Ask vs bid for the whole cluster"; // Ask vs bid v clusteru, berou se vsechny urovne dohromady.
	maximumCountOfSmallVolumesInCluster.SetInt(0);
	maximumCountOfSmallVolumesInCluster.Name = "Maximum count of small volumes in cluster"; // Mozny pocet radku v clusteru, ktere nesplnuji podminku.
	showClusterInAppropriatePartOfCandle.SetYesNo(true);
	showClusterInAppropriatePartOfCandle.Name = "Show cluster in appropriate part of candle"; // Cluster musi byt ve spravne casti svicky (bid cluster v dolni casti, ask cluster v horni casti).
	compareVolumeWithPreviousCandle.SetYesNo(false);
	compareVolumeWithPreviousCandle.Name = "Compare volume with previous candle"; // Je alespon jedno BidVolume vetsi nez nejvetsi BidVolume na predchozi usecce? To same Ask volume.
	inputMinimumImbalanceVolume.SetInt(0);
	inputMinimumImbalanceVolume.Name = "Minimum imbalance volume"; // Rozdil mezi bid a ask volume na danem price levelu musi byt absolutne toto cislo.
	askBidCompareLocalMultiplicatorInputRef.SetDouble(1.1);
	askBidCompareLocalMultiplicatorInputRef.Name = "Ask vs bid on each row"; // Ask vs bid v clusteru, na jednotlivych urovnich.
	inputMinimumVolumeOnFirstLevel.SetInt(100);
	inputMinimumVolumeOnFirstLevel.Name = "Minimum volume on first level"; // Minimalni objem na prvni urovni clusteru.

	askMaxClusterPrice.Name = "askMaxClusterPrice";
	askMaxClusterPrice.DrawStyle = DRAWSTYLE_DASH;
	askMaxClusterPrice.LineWidth = 3;
	askMaxClusterPrice.PrimaryColor = COLOR_LIME;

	askMinClusterPrice.Name = "askMinClusterPrice";
	askMinClusterPrice.DrawStyle = DRAWSTYLE_DASH;
	askMinClusterPrice.LineWidth = 3;
	askMinClusterPrice.PrimaryColor = COLOR_LIME;

	bidMaxClusterPrice.Name = "bidMaxClusterPrice";
	bidMaxClusterPrice.DrawStyle = DRAWSTYLE_DASH;
	bidMaxClusterPrice.LineWidth = 3;
	bidMaxClusterPrice.PrimaryColor = COLOR_RED;

	bidMinClusterPrice.Name = "bidMinClusterPrice";
	bidMinClusterPrice.DrawStyle = DRAWSTYLE_DASH;
	bidMinClusterPrice.LineWidth = 3;
	bidMinClusterPrice.PrimaryColor = COLOR_RED;

	return;
  }

  if ((int)sc.VolumeAtPriceForBars->GetNumberOfBars() < sc.ArraySize)
  {
	return;
  }

  double askBidCompareMultiplicator = askBidCompareMultiplicatorInputRef.GetDouble();
  int askMinimumSizeOfCluster = minimumSizeOfCluster.GetInt();
  int bidMinimumSizeOfCluster = minimumSizeOfCluster.GetInt();
  unsigned int minimumVolume = inputMinimumVolume.GetInt();
  int bidMaximumSmallerVolumes = maximumCountOfSmallVolumesInCluster.GetInt();
  int askMaximumSmallerVolumes = maximumCountOfSmallVolumesInCluster.GetInt();
  int minimumImbalanceVolume = inputMinimumImbalanceVolume.GetInt();
  double askBidCompareLocalMultiplicator = askBidCompareLocalMultiplicatorInputRef.GetDouble();
  unsigned int minimumVolumeOnFirstLevel = inputMinimumVolumeOnFirstLevel.GetInt();

  float bidMinVolumePrice = 0;
  float bidMaxVolumePrice = 0;
  int bidSizeOfCluster = 0;
  bool bidClusterFound = false;
  unsigned int bidBidVolumeSuma = 0;
  unsigned int bidAskVolumeSuma = 0;
  bool bidBiggerVolumeThanPreviousCandle = false;
  int bidSmallerVolumes = 0;

  try
  {

	// Nalezeni nejmensiho bidu na predchozi svicce.
	unsigned int bidMaxVolumePreviousCandle = 0;
	int Count = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(sc.Index - 1);
	for (int ElementIndex = 0; ElementIndex < Count; ElementIndex++)
	{
	  s_VolumeAtPriceV2* p_VolumeAtPriceAtIndex = 0;
	  sc.VolumeAtPriceForBars->GetVAPElementAtIndex(sc.Index - 1, ElementIndex, &p_VolumeAtPriceAtIndex);
	  if (bidMaxVolumePreviousCandle < p_VolumeAtPriceAtIndex->BidVolume)
	  {
		bidMaxVolumePreviousCandle = p_VolumeAtPriceAtIndex->BidVolume;
	  }
	}

	Count = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(sc.Index);
	for (int ElementIndex = 0; ElementIndex < Count; ElementIndex++)
	{
	  s_VolumeAtPriceV2* p_VolumeAtPriceAtIndex = 0;
	  sc.VolumeAtPriceForBars->GetVAPElementAtIndex(sc.Index, ElementIndex, &p_VolumeAtPriceAtIndex);

	  s_VolumeAtPriceV2* p_VolumeAtPriceAtIndexPlusOne = 0;
	  if (ElementIndex + 1 < Count)
	  {
		sc.VolumeAtPriceForBars->GetVAPElementAtIndex(sc.Index, ElementIndex + 1, &p_VolumeAtPriceAtIndexPlusOne);
	  }

	  // if (p_VolumeAtPriceAtIndex && sc.ArraySize - 1)
	  // {
	  //   SCString logMessage;
	  //   logMessage.Format("Index: %d, Price: %f, BidVol: %u, AskVol: %u",
	  //    ElementIndex,
	  //	  p_VolumeAtPriceAtIndex->PriceInTicks * sc.TickSize,
	  //	  p_VolumeAtPriceAtIndex->BidVolume,
	  //	  p_VolumeAtPriceAtIndex->AskVolume);

	  //  sc.AddMessageToLog(logMessage, 0);
	  // }

	  // if (p_VolumeAtPriceAtIndexPlusOne && sc.ArraySize - 1)
	  // {
	  //  SCString logMessage;
	  //  logMessage.Format("*Index: %d, Price: %f, BidVol: %u, AskVol: %u",
	  //	ElementIndex,
	  //	p_VolumeAtPriceAtIndexPlusOne->PriceInTicks * sc.TickSize,
	  //	p_VolumeAtPriceAtIndexPlusOne->BidVolume,
	  //	p_VolumeAtPriceAtIndexPlusOne->AskVolume);

	  // sc.AddMessageToLog(logMessage, 0);
	  // }

	  if (/*p_VolumeAtPriceAtIndex */// Jsou data?
		p_VolumeAtPriceAtIndexPlusOne // Jsou data?
		&& bidMinVolumePrice == 0 // Zadny cluster zatim neni prohledavan.
		&& !bidClusterFound // Zadny cluster neni zatim nalezen.
		&& p_VolumeAtPriceAtIndex->BidVolume >= minimumVolumeOnFirstLevel // Spousteci uroven, minimum value na prvni urovni
		&& p_VolumeAtPriceAtIndex->BidVolume >= minimumVolume // Prvni polozka clusteru ma objem.
		&& (int)p_VolumeAtPriceAtIndex->BidVolume - (int)p_VolumeAtPriceAtIndexPlusOne->AskVolume >= minimumImbalanceVolume // Test minimalni velikosti imbalance (rozdil ve volume mezi bid a ask na dane urovni)
		&& (double)p_VolumeAtPriceAtIndex->BidVolume >= (double)p_VolumeAtPriceAtIndexPlusOne->AskVolume * (double)askBidCompareLocalMultiplicator // Test, zda lokalne je dodrzen nasobek
		)
	  {
		// Pocatek vyhledavani clusteru.
		bidMinVolumePrice = p_VolumeAtPriceAtIndex->PriceInTicks * sc.TickSize;
		bidSizeOfCluster = 0; // Velikost clusteru je nula.
	  }

	  if (/*p_VolumeAtPriceAtIndex*/
		p_VolumeAtPriceAtIndexPlusOne // Jsou data?
		&& bidMinVolumePrice > 0 // Cluster je aktualne prohledavan.
		&& !bidClusterFound // Zadny cluster neni zatim nalezen.		
		&& (p_VolumeAtPriceAtIndex->BidVolume >= minimumVolume  // Aktualni polozka clusteru ma minimalni objem...
		  || bidSmallerVolumes < bidMaximumSmallerVolumes) // nebo jsem neprekrocil mnozstvi urovni, ktere jsou pod objemem.
		// Test minimalni velikosti imbalance (rozdil ve volume mezi bid a ask na dane urovni) nebo jsem neprekrocil mnozstvi urovni, ktere jsou pod objemem.
		&& ((int)p_VolumeAtPriceAtIndex->BidVolume - (int)p_VolumeAtPriceAtIndexPlusOne->AskVolume >= minimumImbalanceVolume || bidSmallerVolumes < bidMaximumSmallerVolumes)
		// Test, zda lokalne je dodrzen nasobek
		&& ((double)p_VolumeAtPriceAtIndex->BidVolume >= (double)p_VolumeAtPriceAtIndexPlusOne->AskVolume * (double)askBidCompareLocalMultiplicator || bidSmallerVolumes < bidMaximumSmallerVolumes)
		)
	  {
		// SCString logMessage;

		// int result = minimumImbalanceVolume > p_VolumeAtPriceAtIndex->BidVolume - p_VolumeAtPriceAtIndexPlusOne->AskVolume;

		// logMessage.Format("Index: %d, Price: %f, BidVol: %u, AskVol: %u, BidVol - AskVol: %u, minimumImbalanceVolume: %u, result: %u",
		//  ElementIndex,
		//  p_VolumeAtPriceAtIndex->PriceInTicks * sc.TickSize,
		//  p_VolumeAtPriceAtIndex->BidVolume,
		//  p_VolumeAtPriceAtIndexPlusOne->AskVolume,
		//  p_VolumeAtPriceAtIndex->BidVolume - p_VolumeAtPriceAtIndexPlusOne->AskVolume,
		//  minimumImbalanceVolume,
		//  result);

		// sc.AddMessageToLog(logMessage, 0);

		// logMessage.Format("Index: %d, Price: %f, BidVol: %u, AskVol: %u",
		// 	ElementIndex,
		// 	p_VolumeAtPriceAtIndex->PriceInTicks * sc.TickSize,
		// 	p_VolumeAtPriceAtIndex->BidVolume,
		// 	p_VolumeAtPriceAtIndex->AskVolume);

		// sc.AddMessageToLog(logMessage, 0);

		// logMessage.Format("*Index: %d, Price: %f, BidVol: %u, AskVol: %u",
		// 	ElementIndex,
		// 	p_VolumeAtPriceAtIndexPlusOne->PriceInTicks * sc.TickSize,
		// 	p_VolumeAtPriceAtIndexPlusOne->BidVolume,
		// 	p_VolumeAtPriceAtIndexPlusOne->AskVolume);

		// sc.AddMessageToLog(logMessage, 0);

		if (p_VolumeAtPriceAtIndex->BidVolume < minimumVolume 
		  || (int)p_VolumeAtPriceAtIndex->BidVolume - (int)p_VolumeAtPriceAtIndexPlusOne->AskVolume < minimumImbalanceVolume
		  || (double)p_VolumeAtPriceAtIndex->BidVolume < (double)p_VolumeAtPriceAtIndexPlusOne->AskVolume * (double)askBidCompareLocalMultiplicator
		  )
		{
		  bidSmallerVolumes++; // Incrementuji minimalni pocet podobjemovych urovni.
		}
		else
		{
		  bidMaxVolumePrice = p_VolumeAtPriceAtIndex->PriceInTicks * sc.TickSize; // Nastav novou pozici druhe strany clusteru.
		  bidSizeOfCluster++; // Zvetsi velikost clusteru.
		  bidBidVolumeSuma += p_VolumeAtPriceAtIndex->BidVolume; // Suma polozek v bidu clusteru.
		  bidAskVolumeSuma += p_VolumeAtPriceAtIndexPlusOne->AskVolume; // Suma polozek v asku clusteru.
		}
		if (p_VolumeAtPriceAtIndex->BidVolume > bidMaxVolumePreviousCandle) // Je alespon jedno BidVolume vetsi nez nejvetsi bid na predchozi usecce?
		{
		  bidBiggerVolumeThanPreviousCandle = true;
		}
	  }
	  else
	  {
		if (bidSizeOfCluster >= bidMinimumSizeOfCluster) // Pokud dosahu patricne velikosti clusteru, nastavim, ze jsem nasel cluster
		{
		  bidClusterFound = true;
		}
		bidSizeOfCluster = 0;
		if (!bidClusterFound)
		{
		  bidSmallerVolumes = 0;
		  bidMinVolumePrice = 0;
		  bidMaxVolumePrice = 0;
		  bidBidVolumeSuma = 0;
		  bidAskVolumeSuma = 0;
		  bidBiggerVolumeThanPreviousCandle = false;
		}
	  }
	}

	float askMaxVolumePrice = 0;
	float askMinVolumePrice = 0;
	int askSizeOfCluster = 0;
	bool askClusterFound = false;
	unsigned int askAskVolumeSuma = 0;
	unsigned int askBidVolumeSuma = 0;
	bool askBiggerVolumeThanPreviousCandle = false;
	int askSmallerVolumes = 0;

	unsigned int askMaxVolumePreviousCandle = 0;
	Count = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(sc.Index - 1);
	for (int ElementIndex = 0; ElementIndex < Count; ElementIndex++)
	{
	  s_VolumeAtPriceV2* p_VolumeAtPriceAtIndex = 0;
	  sc.VolumeAtPriceForBars->GetVAPElementAtIndex(sc.Index - 1, ElementIndex, &p_VolumeAtPriceAtIndex);
	  if (askMaxVolumePreviousCandle < p_VolumeAtPriceAtIndex->AskVolume)
	  {
		askMaxVolumePreviousCandle = p_VolumeAtPriceAtIndex->AskVolume;
	  }
	}

	Count = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(sc.Index);
	for (int ElementIndex = Count - 1; ElementIndex >= 0; ElementIndex--)
	{
	  s_VolumeAtPriceV2* p_VolumeAtPriceAtIndex = 0;
	  sc.VolumeAtPriceForBars->GetVAPElementAtIndex(sc.Index, ElementIndex, &p_VolumeAtPriceAtIndex);

	  s_VolumeAtPriceV2* p_VolumeAtPriceAtIndexMinusOne = 0;
	  if (ElementIndex - 1 >= 0)
	  {
		sc.VolumeAtPriceForBars->GetVAPElementAtIndex(sc.Index, ElementIndex - 1, &p_VolumeAtPriceAtIndexMinusOne);
	  }

	  if (p_VolumeAtPriceAtIndexMinusOne
		  && askMaxVolumePrice == 0
		  && !askClusterFound
		  && p_VolumeAtPriceAtIndex->AskVolume >= minimumVolumeOnFirstLevel // Spousteci uroven, minimum value na prvni urovni
		  && p_VolumeAtPriceAtIndex->AskVolume >= minimumVolume
		  && (int)p_VolumeAtPriceAtIndex->AskVolume - (int)p_VolumeAtPriceAtIndexMinusOne->BidVolume >= minimumImbalanceVolume
		  && (double)p_VolumeAtPriceAtIndex->AskVolume >= (double)p_VolumeAtPriceAtIndexMinusOne->BidVolume * (double)askBidCompareLocalMultiplicator
		)
	  {
		askMaxVolumePrice = p_VolumeAtPriceAtIndex->PriceInTicks * sc.TickSize;
		askSizeOfCluster = 0;
	  }

	  if (/*p_VolumeAtPriceAtIndex*/
		p_VolumeAtPriceAtIndexMinusOne
		&& askMaxVolumePrice > 0
		&& (p_VolumeAtPriceAtIndex->AskVolume >= minimumVolume || askSmallerVolumes < askMaximumSmallerVolumes)
		&& !askClusterFound
		&& ((int)p_VolumeAtPriceAtIndex->AskVolume - (int)p_VolumeAtPriceAtIndexMinusOne->BidVolume >= minimumImbalanceVolume || askSmallerVolumes < askMaximumSmallerVolumes)
		&& ((double)p_VolumeAtPriceAtIndex->AskVolume >= (double)p_VolumeAtPriceAtIndexMinusOne->BidVolume * (double)askBidCompareLocalMultiplicator || askSmallerVolumes < askMaximumSmallerVolumes)
		)
	  {
		if (p_VolumeAtPriceAtIndex->AskVolume < minimumVolume 
		  || (int)p_VolumeAtPriceAtIndex->AskVolume - (int)p_VolumeAtPriceAtIndexMinusOne->BidVolume < minimumImbalanceVolume
		  || (double)p_VolumeAtPriceAtIndex->AskVolume < (double)p_VolumeAtPriceAtIndexMinusOne->BidVolume * (double)askBidCompareLocalMultiplicator
		  )
		{
		  askSmallerVolumes++;
		}
		else
		{
		  askMinVolumePrice = p_VolumeAtPriceAtIndex->PriceInTicks * sc.TickSize;
		  askSizeOfCluster++;
		  askAskVolumeSuma += p_VolumeAtPriceAtIndex->AskVolume;
		  askBidVolumeSuma += p_VolumeAtPriceAtIndexMinusOne->BidVolume;
		}
		if (p_VolumeAtPriceAtIndex->AskVolume > askMaxVolumePreviousCandle)
		{
		  askBiggerVolumeThanPreviousCandle = true;
		}
	  }
	  else
	  {
		if (askSizeOfCluster >= askMinimumSizeOfCluster)
		{
		  askClusterFound = true;
		}
		askSizeOfCluster = 0;
		if (!askClusterFound)
		{
		  askSmallerVolumes = 0;
		  askMaxVolumePrice = 0;
		  askMinVolumePrice = 0;
		  askAskVolumeSuma = 0;
		  askBidVolumeSuma = 0;
		  askBiggerVolumeThanPreviousCandle = false;
		}
	  }
	}

	bool showBidCluster = false;
	if (bidClusterFound
	  && bidBidVolumeSuma > bidAskVolumeSuma * askBidCompareMultiplicator
	  && (bidBiggerVolumeThanPreviousCandle || !compareVolumeWithPreviousCandle.GetBoolean()))
	{
	  showBidCluster = true;
	}

	bool showAskCluster = false;
	if (askClusterFound
	  && askAskVolumeSuma > askBidVolumeSuma * askBidCompareMultiplicator
	  && (askBiggerVolumeThanPreviousCandle || !compareVolumeWithPreviousCandle.GetBoolean()))
	{
	  showAskCluster = true;
	}

	if (showAskCluster && showBidCluster)
	{
	  if (bidMinVolumePrice == askMinVolumePrice && bidMaxVolumePrice == askMaxVolumePrice)
	  {
		if (bidBidVolumeSuma > askAskVolumeSuma)
		{
		  showAskCluster = false;
		}
		else
		{
		  showBidCluster = false;
		}
	  }
	}

	double halfCandlePrice = (sc.BaseDataIn[SC_HIGH][sc.Index] - sc.BaseDataIn[SC_LOW][sc.Index]) / 2 + sc.BaseDataIn[SC_LOW][sc.Index];
	if (showAskCluster && (!showClusterInAppropriatePartOfCandle.GetBoolean() || askMaxVolumePrice >= halfCandlePrice))
	{
	  askMaxClusterPrice[sc.Index] = askMaxVolumePrice;
	  askMinClusterPrice[sc.Index] = askMinVolumePrice;
	}

	if (showBidCluster && (!showClusterInAppropriatePartOfCandle.GetBoolean() || bidMinVolumePrice <= halfCandlePrice))
	{
	  bidMaxClusterPrice[sc.Index] = bidMinVolumePrice;
	  bidMinClusterPrice[sc.Index] = bidMaxVolumePrice;
	}
  }
  catch (const std::exception& e) {
	SCString logMessage;
	logMessage.Format(e.what());
	sc.AddMessageToLog(logMessage, 0);
  }
}