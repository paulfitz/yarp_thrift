
service Motor {

  i32 get_axes();

  i32 get_name(1: i32 iAxisNumber);

  // in YARP, oneway controls default write mode, reply of specified
  // type will still be given if requested.
  oneway bool set_pos(1: i32 iAxisNumber, 2: double fPosition);

  bool set_rel(1: i32 iAxisNumber, 2: double fPosition);
  bool set_vmo(1: i32 iAxisNumber, 2: double fPosition);

  double get_enc(1: i32 iAxisNumber);
  list<double> get_encs();

  bool set_poss(1: list<double> poss);
  bool set_rels(1: list<double> poss);
  bool set_vmos(1: list<double> poss);

  bool set_aen(1: i32 iAxisNumber);
  bool set_adi(1: i32 iAxisNumber);
  bool set_acu(1: i32 iAxisNumber);

  list<double> get_acus();  
}

