// Creates dents
module dent(DL,direction,lbd,mat_z) {
  if (direction == 1) {
    cube([DL-lbd,mat_z-lbd/2,mat_z*2], center=true);
  } else if (direction == 2) {
    cube([mat_z*2-lbd/2,DL-lbd,mat_z*2], center=true);
  } else if (direction == 3) {
    cube([mat_z*2-lbd/2,wd/4-lbd,mat_z*2], center=true);
  } else {
    cube([mat_z-lbd/2,wd/4-lbd,mat_z*2], center=true);
  }
}

// Creates teeth
module tooth(DL,direction,lbd,mat_z) {
  if (direction == 1) {
    cube([DL+lbd,mat_z*2+lbd/2,mat_z], center=true);
  } else if (direction == 2) {
    cube([mat_z*2+lbd/2,DL+lbd,mat_z], center=true);
  } else {
    cube([mat_z+lbd/2,W/4+lbd,mat_z], center=true);
  }
}
