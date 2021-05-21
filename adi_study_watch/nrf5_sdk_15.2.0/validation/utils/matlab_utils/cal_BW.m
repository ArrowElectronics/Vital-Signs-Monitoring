Vin = 0.3;
gain_8233 = 100;
fs = 250;

% figure
% plot(data1);

numData  = xlsread('.\A13H13_ECG_test_result\EcgAppStream_BW_50Hz_0.3Vpp.csv');
numData1 = xlsread('.\A13H13_ECG_test_result\EcgAppStream_BW_40Hz_0.3Vpp.csv');
numData2 = xlsread('.\A13H13_ECG_test_result\EcgAppStream_BW_30Hz_0.3Vpp.csv');
numData3 = xlsread('.\A13H13_ECG_test_result\EcgAppStream_BW_20Hz_0.3Vpp.csv');
numData4 = xlsread('.\A13H13_ECG_test_result\EcgAppStream_BW_10Hz_0.3Vpp.csv');
numData5 = xlsread('.\A13H13_ECG_test_result\EcgAppStream_BW_5Hz_0.3Vpp.csv');
numData6 = xlsread('.\A13H13_ECG_test_result\EcgAppStream_BW_0.5Hz_0.3Vpp.csv');
numData7 = xlsread('.\A13H13_ECG_test_result\EcgAppStream_BW_0.1Hz_0.3Vpp.csv');

data1 = numData(:,2);
data2 = (data1(300:2200)-mean(data1(300:2200)));
data1_1 = numData1(:,2);
data2_1 = (data1_1(300:2200)-mean(data1_1(300:2200)));
data1_2 = numData2(:,2);
data2_2 = (data1_2(300:2200)-mean(data1_2(300:2200)));
data1_3 = numData3(:,2);
data2_3 = (data1_3(300:2200)-mean(data1_3(300:2200)));
data1_4 = numData4(:,2);
data2_4 = (data1_4(300:2200)-mean(data1_4(300:2200)));
data1_5 = numData5(:,2);
data2_5 = (data1_5(300:2200)-mean(data1_5(300:2200)));
data1_6 = numData6(:,2);
data2_6 = (data1_6(300:2200)-mean(data1_6(300:2200)));
data1_7 = numData7(:,2);
data2_7 = (data1_7(300:2200)-mean(data1_7(300:2200)));

[pxx,w]  = periodogram(data1,gausswin(length(data1)),length(data1),fs);
[pxx1,w1]= periodogram(data1_1,gausswin(length(data1_1)),length(data1_1),fs);
[pxx2,w2]= periodogram(data1_2,gausswin(length(data1_2)),length(data1_2),fs);
[pxx3,w3]= periodogram(data1_3,gausswin(length(data1_3)),length(data1_3),fs);
[pxx4,w4]= periodogram(data1_4,gausswin(length(data1_4)),length(data1_4),fs);
[pxx5,w5]= periodogram(data1_5,gausswin(length(data1_5)),length(data1_5),fs);
[pxx6,w6]= periodogram(data1_6,gausswin(length(data1_6)),length(data1_6),fs);
[pxx7,w7]= periodogram(data1_7,gausswin(length(data1_7)),length(data1_7),fs);

figure
subplot(3,3,1)
plot(w,abs(10*log10(pxx)));
%db50 = 10*log10(pxx(length(pxx)*0.5));

subplot(3,3,2)
plot(w1,abs(10*log10(abs(pxx1))));
%db40 = 10*log10(pxx1(round(length(pxx1)*0.4)));

subplot(3,3,3)
plot(w2,abs(10*log10(abs(pxx2))));
%db30 = 10*log10(pxx2(round(length(pxx2)*0.3)));

subplot(3,3,4)
plot(w3,abs(10*log10(abs(pxx3))));
%db20 = 10*log10(pxx3(round(length(pxx3)*0.2)));

subplot(3,3,5)
plot(w4,abs(10*log10(abs(pxx4))));
%db10 = 10*log10(pxx4(round(length(pxx4)*0.1)));

subplot(3,3,6)
plot(w5,abs(10*log10(abs(pxx5))));
%db5 = 10*log10(pxx5(round(length(pxx5)*0.05)));

subplot(3,3,7)
plot(w6,abs(10*log10(abs(pxx6))));
%db0_5 = 10*log10(pxx6(round(length(pxx6)*0.005)));

subplot(3,3,8)
plot(w7,abs(10*log10(abs(pxx7))));
%db0_1 = 10*log10(pxx7(round(length(pxx7)*0.001)));