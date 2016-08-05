include <shapes.scad>
include <laser-functions.scad>
$fn=50;
export = true;

LBD=0.23;
MATZ=3.15;
WOODZ=3.15;

WS2812 = 5;
WS_BORDER = 3;
WS_SPACE = WS2812 + WS_BORDER;
WS_PIPEH = 4;
WS_H = 1.6;

PCBZ = 1.6;
SOFF = 4;
pcbh = PCBZ + SOFF;

number=9;
WIDTH = 40 + (WS_SPACE);
HEIGHT = WIDTH + 10;
DEPTH = ((number+2) * MATZ) + (2 * WOODZ);

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
  diffuser(0);
  translate([0,0,MATZ]) stack(number);
  translate([0,0,MATZ*(number+2)]) face(1);
  translate([0,0,-MATZ]) face(0);
  translate([wd/2+MATZ/2,5+WS_PIPEH/2+LBD,dp/2-MATZ*3/2]) rotate([0,90,0]) side();
  translate([0,0,MATZ*(number+1)]) diffuser(0);
  translate([-wd/2-MATZ/2,5+WS_PIPEH/2+LBD,dp/2-MATZ*3/2]) rotate([0,90,0]) side();
  /* translate([0,ht/2+WS_PIPEH+pcbh+MATZ/2,dp/2-MATZ*3/2]) rotate([90,0,0]) base();*/
  /* translate([0,ht/2+WS_PIPEH+WS_H/2]) rotate([90,0,0]) pixel();*/
  /* translate([0,ht/2+WS_PIPEH+pcbh-SOFF,dp/2-MATZ*3/2]) rotate([90,0,0]) pcb();*/
} else {
  /* projection() base();*/
  /* for (x = [1:1:number]) {*/
  /*   translate([(wd+2)*(x-1),0,0]) projection() diffuser(x);*/
  /* }*/
  /* projection() side();*/
  /* projection() face(0);*/
  /* projection() face(1);*/
  projection() pcb();
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
      if (number % 2 && number != 0) {
        right();
      }
      else if ( number != 0 ) {
        left();
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
    }
  }
}

module pcb() {
  pcbd = dp - (WOODZ * 2) - LBD;
  difference() {
    roundedBox([wd,pcbd,PCBZ],2);
    translate([0,pcbd/2-MATZ*3/2,0]) {
      for (x = [1:1:9]) {
        translate([0,-MATZ*(x-1),0]) {
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
      translate([0,ht/2,0]) cube([wd,WS_PIPEH*2+pcbh*2,MATZ],center=true);
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
    if (front) {
      scale([0.8,0.8,2]) diffuser(0);
    }
  }
}

module side() {
  HFace = ht+WS_PIPEH+pcbh - 5;
  HTY = HFace / XSlots;
  union() {
  difference () {
    cube([dp,HFace,WOODZ],center=true);
    translate([0,-HFace/2,0]) {
      for (y = [HTY:HTY*2:HFace]) {
        translate([dp/2,y,0]) dent(HTY,2,LBD,WOODZ);
        translate([-dp/2,y,0]) dent(HTY,2,LBD,WOODZ);
      }
    }
  }
    translate([-dp/2,0,0]) {
      for (y = [DLZ:DLZ*2:dp]) {
        translate([y,HFace/2,0]) tooth(DLZ,1,LBD,WOODZ);
      }
    }
  }
}

module base() {
  baseW = wd+WOODZ*2;
  difference() {
    cube([baseW,dp,WOODZ],center=true);
    translate([-wd/2,0,0]) {
      for (x = [DLY:DLY*2:baseW]) {
        translate([x,-dp/2,0]) tooth(DLY,1,LBD,WOODZ);
        translate([x,dp/2,0]) tooth(DLY,1,LBD,WOODZ);
      }
    }
    translate([0,-dp/2,0]) {
      for (y = [DLZ:DLZ*2:dp]) {
        translate([-baseW/2,y,0]) dent(DLZ,2,LBD,WOODZ);
        translate([baseW/2,y,0]) dent(DLZ,2,LBD,WOODZ);
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
    translate([0,0,MATZ*(x-1)]) diffuser(x);
  }
}
