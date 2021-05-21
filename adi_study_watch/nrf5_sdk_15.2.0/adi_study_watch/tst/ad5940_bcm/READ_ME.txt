
======================READ ME=======================================

To test BCM Stand alone app in tst folder,


1. Connect Resistance of 100 - 5k Ohm between EB2 and ET1, short EB1,EB2 and ET1 and ET2 as BCM is 4 wire measurement.
2. Run BCM_Test app in tst/ad5940_bcm/ses folder.
3. Wait for 10 seconds for application to generate results.
4. Variable "pImp" gets updated on live watch, which has 2 values being magnitude and phase , first being resistance connected between set of 
Bottom and Top Electrodes for measurement.