$fn=100;

difference() {
    cylinder(50
    , 12, 12, center=true);
    for (i = [1:4]) {
        rotate([0, 0, 90*i]) translate([14, 0, 0])
        cube([12, 12, 51], center=true);
    }
}