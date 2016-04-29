#pragma once

#ifdef __cplusplus
extern "C" {
#endif
  struct BottleStruct;
  typedef struct BottleStruct Bottle;

  typedef struct {
    void (*fini)(Bottle *me);
    double (*getVolume)(const Bottle *me);
    void (*setVolume)(Bottle *me, double value);
  } BottleVftbl;

  BottleVftbl *Bottle_vftbl();

  struct BottleStruct {
    const BottleVftbl *vftbl;
    double volume;
  };

  Bottle *Bottle_new();
  void Bottle_delete(Bottle *me);
  void Bottle_init(Bottle *me);
  void Bottle_fini(Bottle *me);  

  double Bottle_getVolume(const Bottle *me);
  void Bottle_setVolume(Bottle *me,double value);

#ifdef __cplusplus
}
#endif
