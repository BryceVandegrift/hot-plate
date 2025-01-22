$fn = 30;

difference() {
    cylinder(h = 2, d = 5.8);
    translate([0, 0, -0.5]) {
        cylinder(h = 3, d = 3.8 + 0.4);
    }
}