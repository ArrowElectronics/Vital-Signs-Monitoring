Vin  = 0.5;
Vin1 = 0.3;
Vin2 = 0.2;
Vin3 = 0.1;
Freq = 20;
gain_8233 = 150;
fs = 250;

numData  = xlsread('EcgAppStream_DR_5Hz_0.25Vpp.csv');
numData1 = xlsread('EcgAppStream_DR_5Hz_0.15Vpp.csv');
numData2 = xlsread('EcgAppStream_DR_5Hz_0.1Vpp.csv');
numData3 = xlsread('EcgAppStream_DR_5Hz_0.05Vpp.csv');

data1 = numData(:,2);
%data2 = (data1(300:2200)-mean(data1(300:2200)));
data1_1 = numData1(:,2);
%data2_1 = (data1_1(300:2200)-mean(data1_1(300:2200)));
data1_2 = numData2(:,2);
%data2_2 = (data1_2(300:2200)-mean(data1_2(300:2200)));
data1_3 = numData3(:,2);
%data2_3 = (data1_3(300:2200)-mean(data1_3(300:2200)));

Vdata1 = 1.835*((data1(200:1000)/2^15)-1)+1.11;
figure;
subplot(2,2,1)
plot(Vdata1-mean(Vdata1));
Vpp = max(Vdata1)-min(Vdata1);
%Vrms = rms(Vdata2);
%Vinrms = Vin * 0.353;
Gain = Vpp/(Vin/99);
Gain_err = gain_8233 - Gain;
Gain_err_percent = (Gain_err/Gain)*100

Vdata1_1 = 1.835*((data1_1(200:1000)/2^15)-1)+1.11;
% figure;
subplot(2,2,2)
plot(Vdata1_1-mean(Vdata1_1));
Vpp1 = max(Vdata1_1)-min(Vdata1_1);
% Vrms1 = rms(Vdata2_1);
% Vinrms1 = Vin1 * 0.353;
Gain1 = Vpp1/(Vin1/99);
Gain_err1 = gain_8233 - Gain1;
Gain_err_percent1 = (Gain_err1/Gain)*100

Vdata1_2 = 1.835*((data1_2(200:1000)/2^15)-1)+1.11;
% figure;
subplot(2,2,3)
plot(Vdata1_2-mean(Vdata1_2));
Vpp2 = max(Vdata1_2)-min(Vdata1_2);
% Vrms2 = rms(Vdata2_2);
% Vinrms2 = Vin2 * 0.353;
Gain2 = Vpp2/(Vin2/99);
Gain_err2 = gain_8233 - Gain2;
Gain_err_percent2 = (Gain_err2/Gain)*100

Vdata1_3 = 1.835*((data1_3(200:1000)/2^15)-1)+1.11;
% figure;
subplot(2,2,4)
plot(Vdata1_3-mean(Vdata1_3));
Vpp3 = max(Vdata1_3)-min(Vdata1_3);
% Vrms3 = rms(Vdata2_3);
% Vinrms3 = Vin3 * 0.353;
Gain3 = Vpp3/(Vin3/99);
Gain_err3 = gain_8233 - Gain3;
Gain_err_percent3 = (Gain_err3/Gain)*100