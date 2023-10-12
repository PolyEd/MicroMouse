#include <Plotter.h>

const int Xin = A0;
const int Yin = A1;
const int Zin = A2;

float Xbase, Ybase, Zbase;
float X, Y, Z;

Plotter plot;

void setup() {
  delay(100);
  Xbase = analogRead(Xin);
  Ybase = analogRead(Yin);
  Zbase = analogRead(Zin);
  
  plot.Begin();
  plot.AddTimeGraph("ADXL335 Data (Accel-ish)", 500, "X", X, "Y", Y, "Z", Z); // title of graph, zoom level as num of points displayed at once, label of variable, variable
                                                                              // you have to put the thing thats being plotted in a global variable then pass it to this
                                                                              // and update the global in loop()
}

void loop() {
  X = analogRead(Xin) - Xbase;
  Y = analogRead(Yin) - Ybase;
  Z = analogRead(Zin) - Zbase;

  plot.Plot();
}
