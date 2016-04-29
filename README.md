#OOPC - Object Orientd Programming in C


The goal of this project is to demonstrate how to create a single-inheretence
C api which can be wrapped to tranparently work as a C++ API with the same
inheretence.


The base class is Bottle with an effective C++ structure of
```
   struct Bottle {
      double volume;
      Bottle() { volume=0; }
      virtual void setVolume(double value) { volume=value; }
      virtual double getVolume() const { return volume; }
      virtual ~Bottle() {}
   };
```
The derived class is SizedBottle
```
   struct SizedBottle : Bottle {
      double maxVolume;
      SizedBottle() { maxVolume=1; }
      virtual void setVolume(double value) {
        if (value < 0) value=0;
        if (value > maxValue) value=maxValue;
        volume=value;
      }
      virtual void setMaxVolume(double value) { maxVolume=value; }
      virtual double getMaxVolume() const { return maxVolume; }
      virtual ~Bottle() {}
   };
```



