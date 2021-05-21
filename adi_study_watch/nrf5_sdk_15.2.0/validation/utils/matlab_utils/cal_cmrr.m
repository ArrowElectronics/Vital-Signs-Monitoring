clear all;
Vin = 0.3;
Vin1 = 0.5;
Vin2 = 0.2;
Vin3 = 0.1;
Freq = 20;
gain_8233 = 100;
fs = 250;

numData = xlsread('EcgAppStream_cmrr_a.csv');
numData1 = xlsread('EcgAppStream_cmrr_b.csv');
numData2 = xlsread('EcgAppStream_cmrr_c.csv');

data1 = numData(:,2);
data1_1 = numData1(:,2);
data1_2 = numData2(:,2);

figure
subplot(1,3,1)
plot(data1);

subplot(1,3,2)
plot(data1_1);

subplot(1,3,3)
plot(data1_2);

Vdata = 1.835*((data1/2^15)-1)+1.11;
Vrms = rms(Vdata);
Vinrms = (Vin/99)*0.353;
cm = Vrms/Vinrms;
cmrr = 20*log10(cm);

Vdata1 = 1.835*((data1_1/2^15)-1)+1.11;
Vrms1 = rms(Vdata1);
% Vinrms1 = (Vin/99)*0.353;
cm1 = Vrms1/Vinrms;
cmrr1 = 20*log10(cm1);

Vdata2 = 1.835*((data1_2/2^15)-1)+1.11;
Vrms2 = rms(Vdata2);
% Vinrms = (Vin/99)*0.353;
cm2 = Vrms2/Vinrms;
cmrr2 = 20*log10(cm2);

% figure
% plot(data1);
%---------------Switch---------------------------------
% [pxx,w]= periodogram(data2,gausswin(length(data2)),length(data2),fs);
% figure
% subplot(3,3,1)
% plot(w,abs(10*log10(pxx)));
% db50 = 10*log10(pxx(length(pxx)*0.5));
%----------------------------------Bandwidth----------------------

% numData3 = xlsread('./A13H13_ECG_test_result/EcgAppStream_BW_20Hz_0.3Vpp.csv');
% 
% numData4 = xlsread('./A13H13_ECG_test_result/EcgAppStream_BW_10Hz_0.3Vpp.csv');
% numData5 = xlsread('./A13H13_ECG_test_result/EcgAppStream_BW_5Hz_0.3Vpp.csv');
% numData6 = xlsread('./A13H13_ECG_test_result/EcgAppStream_BW_0.5Hz_0.3Vpp.csv');
% numData7 = xlsread('./A13H13_ECG_test_result/EcgAppStream_BW_0.1Hz_0.3Vpp.csv');
% 
% data1_1 = numData1(:,2);
% data2_1 = (data1_1(300:2200)-mean(data1_1(300:2200)));
% data1_2 = numData2(:,2);
% data2_2 = (data1_2(300:2200)-mean(data1_2(300:2200)));
% data1_3 = numData3(:,2);
% data2_3 = (data1_3(300:2200)-mean(data1_3(300:2200)));
% 
% data1_4 = numData4(:,2);
% data2_4 = (data1_4(300:2200)-mean(data1_4(300:2200)));
% data1_5 = numData5(:,2);
% data2_5 = (data1_5(300:2200)-mean(data1_5(300:2200)));
% data1_6 = numData6(:,2);
% data2_6 = (data1_6(300:2200)-mean(data1_6(300:2200)));
% data1_7 = numData7(:,2);
% data2_7 = (data1_7(300:2200)-mean(data1_7(300:2200)));
% 
% [pxx1,w1]= periodogram(data2_1,gausswin(length(data2_1)),length(data2_1),fs);
% [pxx2,w2]= periodogram(data2_2,gausswin(length(data2_2)),length(data2_2),fs);
% [pxx3,w3]= periodogram(data2_3,gausswin(length(data2_3)),length(data2_3),fs);
% [pxx4,w4]= periodogram(data2_4,gausswin(length(data2_4)),length(data2_4),fs);
% [pxx5,w5]= periodogram(data2_5,gausswin(length(data2_5)),length(data2_5),fs);
% [pxx6,w6]= periodogram(data2_6,gausswin(length(data2_6)),length(data2_6),fs);
% [pxx7,w7]= periodogram(data2_7,gausswin(length(data2_7)),length(data2_7),fs);
% 
% subplot(3,3,2)
% plot(w1,abs(10*log10(abs(pxx1))));
% %db40 = 10*log10(pxx1(round(length(pxx1)*0.4)));
% 
% subplot(3,3,3)
% plot(w2,abs(10*log10(abs(pxx2))));
% %db30 = 10*log10(pxx2(round(length(pxx2)*0.3)));
% 
% subplot(3,3,4)
% plot(w3,abs(10*log10(abs(pxx3))));
% %db20 = 10*log10(pxx3(round(length(pxx3)*0.2)));
% 
% subplot(3,3,5)
% plot(w4,abs(10*log10(abs(pxx4))));
% %db10 = 10*log10(pxx4(round(length(pxx4)*0.1)));
% 
% subplot(3,3,6)
% plot(w5,abs(10*log10(abs(pxx5))));
% %db5 = 10*log10(pxx5(round(length(pxx5)*0.05)));
% 
% subplot(3,3,7)
% plot(w6,abs(10*log10(abs(pxx6))));
% %db0_5 = 10*log10(pxx6(round(length(pxx6)*0.005)));
% 
% subplot(3,3,8)
% plot(w7,abs(10*log10(abs(pxx7))));
% %db0_1 = 10*log10(pxx7(round(length(pxx7)*0.001)));
%---------------Noise----------------------------------
% datamax = max(data2);
% datamin = min(data2);
% Vpp = (datamax - datamin)*56*10^-6;
% Vnoise = Vpp/100;
%---------------CMRR--------------------------------
%data2 = data1(17286:21546);
% Vdata = data1 * 56 * 10^-6;
% Vrms = rms(Vdata);
% Vinrms = (Vin/99)*0.353;
% cm = Vrms/Vinrms;
% cmrr = 20*log10(cm);
%---------------dynamic range---------------------------
% %data2_nondc = data2 - mean(data2);
% Vdata2 = data2 * 56 * 10^-6;
% figure;
% subplot(2,2,1)
% plot(Vdata2);
% Vpp = max(Vdata2)-min(Vdata2);
% %Vrms = rms(Vdata2);
% %Vinrms = Vin * 0.353;
% Gain = Vpp/(Vin/99);
% Gain_err = gain_8233 - Gain;
% 
% Vdata2_1 = data2_1 * 56 * 10^-6;
% % figure;
% subplot(2,2,2)
% plot(Vdata2_1);
% Vpp1 = max(Vdata2_1)-min(Vdata2_1);
% % Vrms1 = rms(Vdata2_1);
% % Vinrms1 = Vin1 * 0.353;
% Gain1 = Vpp1/(Vin1/99);
% Gain_err1 = gain_8233 - Gain1;
% 
% Vdata2_2 = data2_2 * 56 * 10^-6;
% % figure;
% subplot(2,2,3)
% plot(Vdata2_2);
% Vpp2 = max(Vdata2_2)-min(Vdata2_2);
% % Vrms2 = rms(Vdata2_2);
% % Vinrms2 = Vin2 * 0.353;
% Gain2 = Vpp2/(Vin2/99);
% Gain_err2 = gain_8233 - Gain2;
% 
% Vdata2_3 = data2_3 * 56 * 10^-6;
% % figure;
% subplot(2,2,4)
% plot(Vdata2_3);
% Vpp3 = max(Vdata2_3)-min(Vdata2_3);
% % Vrms3 = rms(Vdata2_3);
% % Vinrms3 = Vin3 * 0.353;
% Gain3 = Vpp3/(Vin3/99);
% Gain_err3 = gain_8233 - Gain3;