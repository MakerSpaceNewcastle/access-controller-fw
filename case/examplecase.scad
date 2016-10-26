
//space INSIDE box is as follows:


//X is outer_x  minus 4x material thickness
//Y is outer_y minux 4xmaterial thickness
//Z is outer_z minus 2x material_thickness

material_thickness = 3; //in mm

outer_x = 30 + 4* material_thickness; // 235mm 'inside' size.   //Outer size in mm
outer_y = 85 + 4*material_thickness;//100 'inside' dia
outer_z = 55 + 2 * material_thickness;  


x_edge_notches = 2;		//Number of notches along the X side of the box
x_edge_notch_len = 10;

y_edge_notches = 2;		//Number of notches along the Y side of box
y_edge_notch_len = 20;

z_edge_notches = 2;    
z_edge_notch_len = 10;

//sainsmart rfid board size
sainsmart_rf_board_x = 40;
sainsmart_rf_board_y = 60; 

z_edge_gap_between_notches = ( ((outer_z-2*material_thickness) - (z_edge_notch_len * z_edge_notches)) / z_edge_notches);



long_edge_gap_between_notches = (outer_y - (material_thickness*4) -  (y_edge_notch_len * y_edge_notches)) / y_edge_notches  ;
short_edge_gap_between_notches = (outer_x - (material_thickness*2) -  (x_edge_notch_len * x_edge_notches)) / x_edge_notches;


module top_face() {
radius = 3;
   difference() {
		//square([outer_x,outer_y]);
		hull() {

    		translate([(radius/2), (radius/2), 0])
    		circle(r=radius);

   		translate([(outer_x)-(radius/2), (radius/2), 0])
    		circle(r=radius);

    		translate([(radius/2), (outer_y)-(radius/2), 0])
    		circle(r=radius);

   		 translate([(outer_x)-(radius/2), (outer_y)-(radius/2), 0])
   		 circle(r=radius);		
		}


		for (a=[0:y_edge_notches-1]) {
			translate([material_thickness,  material_thickness*2 + ((long_edge_gap_between_notches * a ) + (a * y_edge_notch_len) +  (0.5 * long_edge_gap_between_notches)) ,0]  ) square([material_thickness,y_edge_notch_len]);
   	}
	
		for (a=[0:y_edge_notches-1]) {
			translate([outer_x - ( material_thickness*2),  (2*material_thickness + (long_edge_gap_between_notches * a ) + (a * y_edge_notch_len) +  (0.5 * long_edge_gap_between_notches)) ,0]  ) square([material_thickness,y_edge_notch_len]);
		}

	for (a=[0:x_edge_notches-1]) {
			
translate([material_thickness + x_edge_notch_len + 0.5*short_edge_gap_between_notches + x_edge_notch_len*a + a*short_edge_gap_between_notches,material_thickness,0]) rotate([0,0,90]) square([material_thickness, x_edge_notch_len]);
		}

	for (a=[0:x_edge_notches-1]) {
			
translate([material_thickness + x_edge_notch_len + 0.5*short_edge_gap_between_notches + x_edge_notch_len*a + a*short_edge_gap_between_notches,outer_y - (material_thickness*2),0]) rotate([0,0,90]) square([material_thickness, x_edge_notch_len]);
		}

	}
}

module long_side_face() {
	union() {
		square([outer_y-(material_thickness*4), outer_z - (material_thickness*2)]);
		//lugs for the long edge
		for (a=[0:y_edge_notches-1]) {
			translate([((long_edge_gap_between_notches * a ) + (a * y_edge_notch_len) +  (0.5 * long_edge_gap_between_notches)),-material_thickness,0]) square([y_edge_notch_len, outer_z]);
		}

			}
		for (a=[0:z_edge_notches - 1 ]) {
			translate([-material_thickness,(0.5 * z_edge_gap_between_notches) + (z_edge_notch_len * a) + (a*z_edge_gap_between_notches),0]) square([material_thickness, z_edge_notch_len]);

			translate([outer_y - 4*material_thickness,(0.5 * z_edge_gap_between_notches) + (z_edge_notch_len * a) + (a*z_edge_gap_between_notches),0]) square([material_thickness, z_edge_notch_len]);
		}



}

module short_side_face() {
	difference() {
		union() {
			square([outer_x - (2*material_thickness), outer_z-(2*material_thickness)]);
			//lugs
			for (a=[0:x_edge_notches-1]) {
				translate([0.5*short_edge_gap_between_notches + x_edge_notch_len*a + a*short_edge_gap_between_notches,-material_thickness,0]) square([x_edge_notch_len,outer_z]);
			}
		}

		//Z notches
		for (a=[0:z_edge_notches - 1 ]) {
			translate([0,(0.5 * z_edge_gap_between_notches) + (z_edge_notch_len * a) + (a*z_edge_gap_between_notches),0]) square([material_thickness, z_edge_notch_len]);

			translate([outer_x - 3*material_thickness,(0.5 * z_edge_gap_between_notches) + (z_edge_notch_len * a) + (a*z_edge_gap_between_notches),0]) square([material_thickness, z_edge_notch_len]);
		}
	}
}


hole_size=4;
nixie_screw = 3.5;
nixie_tube_dia = 19;
    
first_board_x_offset = material_thickness*2 + 12.5;
board_y_offset = material_thickness*2+2;

difference() {
	top_face();	
	translate([5+material_thickness*2,outer_y/2,0]) square([2,4]);
	translate([outer_x - (5+material_thickness*2 ),outer_y/2,0]) square([2,4]);
}

translate([0,outer_y + 5,0]) difference() {
	top_face();
	translate([5+material_thickness*2,outer_y/2,0]) square([2,4]);
	translate([outer_x - (5+material_thickness*2 ),outer_y/2,0]) square([2,4]);
}


translate([material_thickness,outer_y*2 + 5 + material_thickness*2 + 3,0]) long_side_face();

translate([material_thickness + outer_y,outer_y*2 + 5 + material_thickness*2 + 3,0]) difference() {

 long_side_face();
translate([material_thickness+7.5,5+material_thickness+7.5,0]) circle(r=1.7);
translate([material_thickness+7.5,5+material_thickness+40-7.5,0]) circle(r=1.7);


translate([material_thickness+44.5,5+material_thickness+3,0]) circle(r=1.7);
translate([material_thickness+44.5,5+material_thickness+40-3,0]) circle(r=1.7);
}



translate([outer_x + material_thickness, material_thickness,0]) difference() {

    difference() {
        short_side_face();
        //Cable entry slot
        hull() {
            translate([(outer_x - material_thickness*2)/2,0,0]) circle(r=1.5, $fn=20);
            translate([(outer_x - material_thickness*2)/2,(outer_z - material_thickness*2)/2,0]) circle(r=1.5, $fn=20);
        }
    }   
}


translate([outer_x + material_thickness , outer_z + 2*material_thickness, ,0]) difference() {
short_side_face();
}