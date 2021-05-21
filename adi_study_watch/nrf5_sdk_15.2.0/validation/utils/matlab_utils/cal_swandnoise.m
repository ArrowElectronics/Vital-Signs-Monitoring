Vin = 0.3;
Freq = 20;
gain_8233 = 150;
fs = 250;

numData = xlsread('EcgAppStream_SwitchON.csv');%_SwitchON.csv');
numData1 = xlsread('EcgAppStream_SwitchOFF.csv');
numData2 = xlsread('EcgAppStream_ad8233_noise.csv');%('.\A11H11_ECG_test_result\EcgAppStream_ad8233_noise.csv');

data1 = numData(:,2);
%data2 = (data1(300:2000)-mean(data1(300:2000)));
data1_1 = numData1(:,2);
%data2_1 = (data1_1(300:2000)-mean(data1_1(300:2000)));
data1_2 = numData2(:,2);
%data2_2 = (data1_2(fs*49:fs*61)-mean(data1_2(fs*49:fs*61)));
% figure
% plot(data1);
%---------------Switch---------------------------------
Vdata1 = 1.835*((data1/2^15)-1)+1.11;
figure
subplot(2,1,1)
plot(Vdata1-mean(Vdata1));
Vpp_switchON = (max(Vdata1(300:1000)) - min(Vdata1(300:1000)))*99/150
Vdata2 = 1.835*((data1_1/2^15)-1)+1.11;
[pxx,w]= periodogram(Vdata2,gausswin(length(Vdata2)),length(Vdata2),fs);
subplot(2,1,2)
plot(w,10*log10(pxx));
%---------------Noise----------------------------------
Vdata3 = 1.835*((data1_2(100:length(data1_2))/2^15)-1)+1.11;
Vpp    = [max(Vdata3)-min(Vdata3)];
Vnoise = Vpp/150
figure
plot(data1_2);