/* Copyright JBR Engineering Research Ltd - 2016 - www.jbrengineering.co.uk*/

/* Licensed under GPL 3.0 - a copy of which should have been supplied with this file*/

include <shapes.scad>
include <laser-functions.scad>
include <scad-utils/morphology.scad>
$fn=50;

export = true;
enumbers = false;
eblank = false;
pcb = false;

teeth = false;
node = true;

LBD=0.23; // general kerf diameter
MATZ=3.00; // acrylic thickness
WOODZ=3.15; // wood thickness

WS2812 = 5;
WS_BORDER = 3;
WS_SPACE = WS2812 + WS_BORDER;
WS_PIPEH = 4;
WS_H = 1.6;

PCBZ = 1.6;
SOFF = 4;
pcbh = PCBZ + SOFF + WS_H;
PCB_TABZ = 10;
bspace = 8;
tab = true;

SCREWDIA = 3 + 0.2;
STUDDIA = 5;

number=10;
WIDTH = 40 + (WS_SPACE);
HEIGHT = WIDTH + 10;
DEPTH = ((number+1) * MATZ) + (2 * WOODZ);

ht = HEIGHT + LBD;
wd = WIDTH + LBD;
dp = DEPTH + LBD;
// Slot
// Tweak these till it looks right
XSlots = 6; // number of slots in base
YSlots = 6; // number of slots on sides
ZSlots = 4;
// dent dia in each axis
DLX = (ht / XSlots);
DLY = (wd / YSlots);
DLZ = (dp / ZSlots);

if (!export) {
  diffuser(-1);
  translate([0,0,MATZ]) stack(number);
  translate([0,0,MATZ*(number+1)]) face(1);
  translate([0,0,-WOODZ]) face(0);
  /* translate([wd/2+MATZ/2,5+WS_PIPEH/2+LBD,dp/2-MATZ*3/2]) rotate([0,90,0]) side();*/
  translate([0,0,MATZ*(number)]) diffuser(-1);
  /* translate([-wd/2-MATZ/2,5+WS_PIPEH/2+LBD,dp/2-MATZ*3/2]) rotate([0,90,0]) side();*/
  /* translate([0,ht/2+WS_PIPEH+pcbh+MATZ/2,dp/2-MATZ*3/2]) rotate([90,0,0]) base();*/
  /* translate([0,ht/2+WS_PIPEH+WS_H/2]) rotate([90,0,0]) pixel();*/
  translate([0,ht/2+WS_PIPEH+WS_H,dp/2-MATZ*3/2]) rotate([90,180,0]) pcb();
  /* translate([148.5,45,-89.2]) rotate([90,180,0]) import("nixie-pipe-pcb.stl");*/
  echo ("<b>Height:</b>",ht,"<b>Width:</b>",wd,"<b>Depth:</b>",dp);
} else {
  if (enumbers) {
    translate([0,0,0]) projection() diffuser(-1);
    for (x = [1:1:number]) {
      translate([(wd+2)*(x),0,0]) projection() diffuser(x);
    }
  } else if (eblank) {
    projection() diffuser(-1);
  } else if (pcb) {
    projection() pcb();
    
  } else {
    if (teeth) {
      projection() side();
      translate([dp+1,0,0]) {
        projection() side();
        translate([(dp+1)+MATZ*2+1,0,0]) {
          projection() face(0);
          translate([wd+1+MATZ*2,0,0]) projection() face(1);
        }
      }
    } else {
      projection() face(0);
      translate([wd+1,0,0]) projection() face(1);
    }
  }
}

module middle() {
  translate([0,ht/2,0]) hexagon(WS_PIPEH*2,MATZ);
}

module left() {
  translate([-WS_SPACE/2,ht/2,0]) hexagon(WS_PIPEH*2,MATZ);
}

module right() {
  translate([WS_SPACE/2,ht/2,0]) hexagon(WS_PIPEH*2,MATZ);
}

module diffuser(number) {
  difference() {
    union() {
      roundedBox([wd,ht,MATZ],5);
      if (number % 2 && number > 0) {
        left();
      }
      else if ( number > 0 ) {
        right();
      }
    }
    if (number == 1) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("1",size=38,halign="center",valign="center");
    } else if (number == 2) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("2",size=38,halign="center",valign="center");
    } else if (number == 3) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("3",size=38,halign="center",valign="center");
    } else if (number == 4) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("4",size=38,halign="center",valign="center");
    } else if (number == 5) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("5",size=38,halign="center",valign="center");
    } else if (number == 6) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("6",size=38,halign="center",valign="center");
    } else if (number == 7) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("7",size=38,halign="center",valign="center");
    } else if (number == 8) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("8",size=38,halign="center",valign="center");
    } else if (number == 9) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("9",size=38,halign="center",valign="center");
    } else if (number == 10) {
      rotate([180,180,0]) translate([0,0,-MATZ]) linear_extrude(height = MATZ*3) text("0",size=38,halign="center",valign="center");
    }
    if ((abs(number) >= 1)) {
      if (!teeth) {
        screw_holes(SCREWDIA);
      }
    }
  }
}

module pcb() {
  pcbd = dp - (WOODZ * 2) - LBD;
  difference() {
    union() {
      roundedBox([wd,pcbd,PCBZ],2);
      if (tab) {
        translate([0,-pcbd/2-PCB_TABZ/2,0]) roundedBox([wd-10,WOODZ*2+PCB_TABZ,PCBZ],2);
      } else {
        translate([0,-pcbd/2,0]) roundedBox([wd-10,WOODZ*2,PCBZ],0.5);
      }
      translate([0,pcbd/2,0]) roundedBox([wd-10,WOODZ*2,PCBZ],0.5);
    }
    translate([0,-pcbd/2+MATZ*3/2,0]) {
      for (x = [1:1:number]) {
        translate([0,+MATZ*(x-1),0]) {
          if (x % 2) {
            translate([WS_SPACE/2,0,0]) cube([WS2812,WS2812,PCBZ],center=true);
          } else {
            translate([-WS_SPACE/2,0,0]) cube([WS2812,WS2812,PCBZ],center=true);
          }
        }
      }
    }
  }
}

module face(front) {
  WEnd = wd;
  HFace = ht+WS_PIPEH+pcbh - 5;
  HTY = HFace / XSlots;
  difference() {
    union() {
      diffuser(0);
      if (teeth) {
        translate([0,ht/2,0]) cube([wd,WS_PIPEH*2+pcbh*2,MATZ],center=true);
      } else {
        translate([0,ht/2,0]) roundedBox([wd,WS_PIPEH*2+pcbh*2+bspace,MATZ],5);
      }
      if (teeth) {
        translate([-wd/2,ht/2+WS_PIPEH+pcbh,0]) {
          for (x = [DLY:DLY*2:WEnd]) {
            translate([x,0,0]) tooth(DLY,1,LBD,WOODZ);
          }
        }
        translate([0,-ht/2+5,0]) {
          for (y = [HTY:HTY*2:ht]) {
            translate([-wd/2,y,0]) tooth(HTY,2,LBD,WOODZ);
            translate([wd/2,y,0]) tooth(HTY,2,LBD,WOODZ);
          }
        }
      }
      if (front == 0 && node) {
        translate([0,10,0]) node(0);
        translate([0,-10,0]) node(0);
      }
    }
    if (front) {
      scale([0.8,0.8,2]) diffuser(0);
    }
    // pcb slot
    if (front) {
      translate([0,ht/2+WS_PIPEH+WS_H+1,0]) {
        cube([wd-10+LBD,PCBZ-LBD+0.2,MATZ],center=true);
          // holes on edge provide strain relief
          translate([(wd-10)/2,(PCBZ-LBD)/2,-MATZ]) cylinder(r=0.4,h=MATZ*2);
          translate([(wd-10)/2*-1,(PCBZ-LBD)/2,-MATZ]) cylinder(r=0.4,h=MATZ*2);
          translate([(wd-10)/2,(PCBZ-LBD)/2*-1,-MATZ]) cylinder(r=0.4,h=MATZ*2);
          translate([(wd-10)/2*-1,(PCBZ-LBD)/2*-1,-MATZ]) cylinder(r=0.4,h=MATZ*2);
      }
    } else { // slightly larger for rear acrylic
      translate([0,ht/2+WS_PIPEH+WS_H+1,0]) {
        cube([wd-10+LBD,PCBZ-LBD+0.2,MATZ],center=true);
          // holes on edge provide strain relief
          translate([(wd-10)/2,(PCBZ-LBD)/2,-MATZ]) cylinder(r=0.4,h=MATZ*2);
          translate([(wd-10)/2*-1,(PCBZ-LBD)/2,-MATZ]) cylinder(r=0.4,h=MATZ*2);
          translate([(wd-10)/2,(PCBZ-LBD)/2*-1,-MATZ]) cylinder(r=0.4,h=MATZ*2);
          translate([(wd-10)/2*-1,(PCBZ-LBD)/2*-1,-MATZ]) cylinder(r=0.4,h=MATZ*2);
      }
      if (node) {
        translate([0,10,0]) node(1);
        translate([0,-10,0]) node(1);
        /* translate([0,10,0]) magnet(1);*/
        /* translate([0,-10,0]) magnet(1);*/
        /* translate([0,10,0]) magnet(0);*/
        /* translate([0,-10,0]) magnet(0);*/
      }
    }

    if (!teeth) {
      if (front) {
        screw_holes(SCREWDIA);
      } else {
        screw_holes(STUDDIA);
      }
    }
  }
}

module side() {
  HFace = ht+WS_PIPEH+pcbh - 5;
  HTY = HFace / XSlots;
  union() {
  difference () {
    cube([dp,HFace,WOODZ],center=true);
    if (teeth) {
      translate([0,-HFace/2,0]) {
        for (y = [HTY:HTY*2:HFace]) {
          translate([dp/2,y,0]) dent(HTY,2,LBD,WOODZ);
          translate([-dp/2,y,0]) dent(HTY,2,LBD,WOODZ);
        }
      }
    }
  }
    if (teeth) {
      translate([-dp/2,0,0]) {
        for (y = [DLZ:DLZ*2:dp]) {
          translate([y,HFace/2,0]) tooth(DLZ,1,LBD,WOODZ);
        }
      }
    }
  }
}

module base() {
  baseW = wd+WOODZ*2;
  difference() {
    cube([baseW,dp,WOODZ],center=true);
    if (teeth) {
      translate([-wd/2,0,0]) {
        for (x = [DLY:DLY*2:baseW]) {
          translate([x,-dp/2,0]) tooth(DLY,1,LBD,WOODZ);
          translate([x,dp/2,0]) tooth(DLY,1,LBD,WOODZ);
        }
      }
    }
    if (teeth) {
      translate([0,-dp/2,0]) {
        for (y = [DLZ:DLZ*2:dp]) {
          translate([-baseW/2,y,0]) dent(DLZ,2,LBD,WOODZ);
          translate([baseW/2,y,0]) dent(DLZ,2,LBD,WOODZ);
        }
      }
    }
  }
}

module pixel() {
  difference() {
    cube([WS2812,WS2812,WS_H],center=true);
    translate([0,0,0.5]) cylinder(r=1.5,h=WS_H-1);
  }
}

module stack(number) {
  for (x = [1:1:number]) {
    translate([0,0,MATZ*(number-x)]) diffuser(x);
  }
}

module frame(front) {
  difference() {
    /* roundedBox([(WIDTH*4)-LBD,ht,MATZ],5);*/
    /* scale([0.95,0.8,2]) roundedBox([(WIDTH*4)-LBD,ht,MATZ],5);*/
    /* translate([0,-37,0]) face(1);*/
    roundedBox([(WIDTH*4)-LBD,pcbh*2,MATZ],5);
    translate([5,0,0]) cube([wd-10+LBD,PCBZ-LBD,MATZ],center=false);
    translate([WIDTH+5,0,0]) cube([wd-10+LBD,PCBZ-LBD,MATZ],center=false);
    /* translate([-5,0,0]) cube([wd-10+LBD,PCBZ-LBD,MATZ],center=false);*/
    translate([-WIDTH+5,0,0]) cube([wd-10+LBD,PCBZ-LBD,MATZ],center=false);
  }
}


module screw_holes(dia) {
  hloc = (5+3.2-LBD) / 2;
  translate([wd/2-hloc,ht/2-hloc,0]) cylinder(r=(dia-LBD)/2,h=MATZ,center=true);
  translate([-wd/2+hloc,-ht/2+hloc,0]) cylinder(r=(dia-LBD)/2,h=MATZ,center=true);
  translate([wd/2-hloc,-ht/2+hloc,0]) cylinder(r=(dia-LBD)/2,h=MATZ,center=true);
  translate([-wd/2+hloc,ht/2-hloc,0]) cylinder(r=(dia-LBD)/2,h=MATZ,center=true);
}

module node(cut) {
  if (cut) {
    translate([wd/2-2,0,0]) {
      translate([1,0,0]) cube([2,2.1-LBD,MATZ],center=true);
      translate([0,0,-MATZ]) cylinder(r=2.1-LBD/2,h=MATZ*2);
      translate([-0.5,0,0]) cube([1,4.2-LBD,MATZ],center=true);
      translate([-1,0,-MATZ]) cylinder(r=2.1-LBD/2,h=MATZ*2);
    }
  } else {
    translate([-wd/2-2,0,0]) {
      translate([1,0,0]) cube([2,2+LBD/2,MATZ],center=true);
      cylinder(r=2,h=MATZ);
    }
  }
}

module magnet(cut) {
  if (cut) {
    translate([wd/2-2+LBD,0,0]) {
      translate([1,0,0]) cube([2,2-LBD,MATZ],center=true);
      translate([0,0,-MATZ]) cylinder(r=2-LBD/2,h=MATZ*2);
    }
  } else {
    translate([-wd/2+2-LBD,0,0]) {
      translate([-1,0,0]) cube([2,2-LBD,MATZ],center=true);
      translate([0,0,-MATZ]) cylinder(r=2-LBD/2,h=MATZ*2);
    }
  }
}
