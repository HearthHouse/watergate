wall_thickness = 2;
outer_width = 57 + 2*wall_thickness;
outer_length = 76 + 2*wall_thickness;
outer_height = 56 + 2*wall_thickness;

module hole() {
  translate([7.5, 7.5, 2.5]) difference() {
    cube(size=[15, 15, 5], center=true);
    cylinder(r=3, h=30, center=true);
  }
}

module spanner() {
  translate([7.5, 7.5, 2.5]) difference() {
    difference() {
      translate([7.50,-7.5,0]) rotate(45,[0,0,1]) cube(size=[21.21, 21.21, 5], center=true);
      cube(size=[50, 15, 10], center=true);
    }
  }
}

rotate(90, [1,0,0]) {
  difference() {
    translate(-outer_width/2, -outer_length/2, 0) {
      difference() {
        cube(size=[outer_width, outer_length, outer_height], center=true);
        translate([0,2,0]) cube(size=[outer_width-4, outer_length, outer_height-4], center=true);
      }
    }

    translate([-outer_width/2+2+10, -outer_length/2-2, -outer_height/2+2+10]) cube(size=[12, 5, 12]);
    translate([outer_width/2-16.47, -outer_length/2-2, -outer_height/2+2+10]) cube(size=[8.86, 5, 11.16]);
    translate([outer_width/2-19.87, -outer_length/2-2, -outer_height/2+2+10+12.66]) cube(size=[16, 5, 13.5]);

    translate([0,-outer_length/2-2, 10]) cube(size=[5, 16, 15]);
    translate([-15,-outer_length/2-2, 10]) cube(size=[5, 16, 15]);

  }
  difference() {
    union() {
      translate([outer_width/2, -outer_length/2, -outer_height/2]) hole();
      translate([-outer_width/2-15, -outer_length/2, -outer_height/2]) hole();
      translate([outer_width/2, outer_length/2-15, -outer_height/2]) {
        hole();
        translate([-15,0,0])spanner();
      }
      translate([-outer_width/2-15, outer_length/2-15, -outer_height/2]) {
        hole(); spanner();
      }
    }

    translate(-outer_width/2, 2-outer_length/2, 0) {
      cube(size=[outer_width-4, outer_length, outer_height-4], center=true);
    }
  }
  translate([outer_width, -outer_length/2+2, -outer_height/2+2]) {
    cube(size=[outer_width-wall_thickness*2, 4, outer_height-wall_thickness*2]);
    translate([-2,-2,-2]) cube(size=[outer_width, 2, outer_height]);
  }
}
