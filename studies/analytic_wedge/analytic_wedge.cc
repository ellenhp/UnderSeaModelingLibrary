/**
 * @file analytic_wedge.cc
 *
 * Compares the analytic results of the ASA wedge to the results
 * generated by WaveQ3D under similar conditions.
 *
 */
#include <usml/waveq3d/waveq3d.h>
#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <stdio.h>
#include <stdlib.h>

using namespace usml::waveq3d ;
using namespace usml::ocean ;

/**
 * The following test is to compare the results produced by
 * WaveQ3D to results obtained from the Analytic Wedge problem
 * produced by Deane and Tindle.
 */
int main() {
    cout << "=== analytic_wedge ===" << endl ;

    // define scenario parameters
    int num_targets = 201 ;
    double m_2_deg = 1.0 / ( 1852.0 * 60.0 ) ;            // conversion factor from meters to degree lat
    const double wedge_length = 4000.0 ;
    const double wedge_depth = 200.0 ;
    const double wedge_angle = atan( wedge_depth / wedge_length ) ;
    const double dist = wedge_length * m_2_deg ;
    wposition::compute_earth_radius( 0.0 ) ;
    wposition1 wedge_apex ;
    wedge_apex.latitude(dist) ;
    wposition1 pos ;
    pos.altitude(-100.0) ;

    /// setup fan parameters
    seq_rayfan de ;
//    seq_linear az( 0.0, 15.0, 360.0 ) ;
//    seq_linear de( -89.0, 1.0, 89.0 ) ;
    seq_linear az( 0.0, 0.1, 100.0 ) ;
    seq_log freq( 250.0, 250.0, 1 ) ;
    const double time_max = 7.0 ;
    const double time_step = 0.05 ;

    // setup files to output all data to
    const char* csvname = USML_STUDIES_DIR "/analytic_wedge/analytic_wedge_eigenray.csv";
    const char* ncname = USML_STUDIES_DIR "/analytic_wedge/analytic_wedge_proploss.nc";
    const char* ncname_wave = USML_STUDIES_DIR "/analytic_wedge/analytic_wedge_eigenray_wave.nc";
//    const char* csvname = USML_STUDIES_DIR "/analytic_wedge/analytic_wedge_cxslope_eigenray.csv";
//    const char* ncname = USML_STUDIES_DIR "/analytic_wedge/analytic_wedge_cxslope_proploss.nc";
//    const char* ncname_wave = USML_STUDIES_DIR "/analytic_wedge/analytic_wedge_cxslope_eigenray_wave.nc";

    // build sound velocity profile
    const double c0 = 1500.0 ;
    attenuation_model* att_mod = new attenuation_constant(0.0);
    profile_model*  profile = new profile_linear(c0, att_mod) ;
    profile->flat_earth(true);

    // Create rayleigh model similar to the ASA wedge geophysical params
    reflect_loss_model* asa_wedge = new reflect_loss_rayleigh( 1.5, 1700.0/c0, 0.5) ;
    boundary_model* bottom  = new boundary_slope( wedge_apex, 0.0, wedge_angle, 0.0, asa_wedge ) ;
//    boundary_model* bottom  = new boundary_slope( wedge_apex, 0.0, wedge_angle ) ;
//    bottom->reflect_loss( new reflect_loss_constant(0.0) ) ;

    boundary_model* surface = new boundary_flat() ;

    ocean_model ocean( surface, bottom, profile ) ;

    // initialize proploss targets and wavefront
        // cross-slope targets
    wposition target( num_targets, 1, pos.latitude(), pos.longitude(), -30.0 ) ;
    double inc = ( 6000.0 * m_2_deg ) / num_targets ;
    for( int n=1; n < target.size1(); ++n ) {
        target.longitude( n, 0, inc * n ) ;
    }
        // Up slope targets
//    wposition target( num_targets, 1, pos.latitude(), pos.longitude(), -30.0 ) ;
//    double inc = ( 3400.0 * m_2_deg ) / num_targets ;
//    for( int n=1; n < target.size1(); ++n ) {
//        target.latitude( n, 0, inc * n );
////        cout << "target(" << n << ",0) dist src_lat apex: " << target.latitude(n,0) / m_2_deg << endl;
////        double dist = target.latitude(n,0) / m_2_deg ;
////        target.altitude( n, 0, -100 + (dist * tan(wedge_angle / 2.0)) ) ;
////        cout << "target.alt(" << n << ",0): " << target.altitude(n,0) << endl;
//    }
        // diagonally 45 degrees up slope targets
//    wposition target( num_targets, 1, pos.latitude(), pos.longitude(), -30.0 ) ;
//    double inc = 3400.0 / num_targets ;
//    double bearing = M_PI / 4.0 ;
//    for( int n=1; n < target.size1(); ++n ) {
//        wposition1 aTarget( pos, (inc * n), bearing ) ;
//        target.latitude( n, 0, aTarget.latitude() ) ;
//        target.longitude( n, 0, aTarget.longitude() ) ;
//    }
        // one target only
//    wposition target( 1, 1, pos.latitude() + 1000.0*m_2_deg, pos.longitude(), -30.0 ) ;
    proploss loss( freq, pos, de, az, time_step, &target ) ;
    wave_queue wave( ocean, freq, pos, de, az, time_step, &target ) ;
    wave.addProplossListener( &loss ) ;

    // propagate wavefront
    cout << "writing wavefronts to " << ncname_wave << endl;

    wave.init_netcdf( ncname_wave );
    wave.save_netcdf();

    while ( wave.time() < time_max ) {
        wave.step() ;
        wave.save_netcdf();
    }

    wave.close_netcdf();

    //compute coherent propagation loss and write eigenrays to disk

    loss.sum_eigenrays();
    cout << "writing proploss to " << ncname << endl;
    loss.write_netcdf(ncname,"ASA Anaylytic Wedge");

    //save results to spreadsheet and compare to analytic results

    cout << "writing tables to " << csvname << endl;
    std::ofstream os(csvname);
    os << "target,depth,range,intensity" << endl;
    os << std::setprecision(18);
    cout << std::setprecision(18);

    for(int n = 0; n < target.size1() ; ++n)
    {
        os << n << ","
           << target.altitude(n,0) << ","
           << target.latitude(n,0) / m_2_deg << ","
           << -loss.total(n,0)->intensity(0)
           << endl;
    }
//    std::ofstream os(csvname);
//    os << "time,launch de,target de,intensity,phase,surface,bottom" << endl;
//    os << std::setprecision(18);
//    cout << std::setprecision(18);
//
//    eigenray_list::const_iterator ray ;
//    for(ray = loss.eigenrays(0,0)->begin(); ray != loss.eigenrays(0,0)->end(); ++ray)
//    {
//        os << (*ray).time << ","
//           << (*ray).source_de << ","
//           << (*ray).target_de << ","
//           << -(*ray).intensity(0) << ","
//           << (*ray).phase(0) << ","
//           << (*ray).surface << ","
//           << (*ray).bottom
//           << endl ;
//    }
}
