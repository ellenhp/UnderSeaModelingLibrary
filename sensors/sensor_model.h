/**
 * @file sensor_model.h
 * Instance of an active sensor in the simulation.
 */
#pragma once

#include <usml/eigenverb/eigenverb_collection.h>
#include <usml/eigenverb/wavefront_listener.h>
#include <usml/sensors/receiver_params.h>
#include <usml/sensors/sensor_listener.h>
#include <usml/sensors/orientation.h>
#include <usml/sensors/source_params.h>
#include <usml/sensors/xmitRcvModeType.h>
#include <usml/threads/thread_task.h>
#include <usml/waveq3d/eigenray_collection.h>
#include <set>

namespace usml {
namespace sensors {

using namespace usml::waveq3d;
using namespace usml::eigenverb;
using namespace usml::threads;

/// @ingroup sensors
/// @{

/**
 * Instance of an active sensor in the simulation.
 * As the sensor moves all required attributes are updated. If the attributes
 * change beyond established thresholds a new reverb generation is started.
 */
class USML_DECLSPEC sensor_model: public wavefront_listener {
public:

    /**
     * Data type used for sensorID.
     */
    typedef int id_type;

    /**
     * Construct a new instance of a specific sensor type.
     * Sets the position and orientation values to NAN.
     * These values are not set until the update_sensor()
     * is invoked for the first time.
     *
     * @param sensorID        Identification used to find this sensor instance
     *                         in sensor_manager.
     * @param paramsID        Identification used to lookup sensor type data
     *                         in source_params_map and receiver_params_map.
     * @param description    Human readable name for this sensor instance.
     */
    sensor_model( sensor_model::id_type sensorID, sensor_params::id_type paramsID,
            const std::string& description = std::string());

    /**
     * Removes a sensor instance from simulation.
     * Automatically aborts wavefront task if one exists.
     */
    virtual ~sensor_model() ;

    /**
     * Identification used to find this sensor instance in sensor_manager.
     * @return sensorID for this sensor_model.
     */
    id_type sensorID() const {
        return _sensorID;
    }

    /**
     * Identification used to lookup sensor type data in 
     * source_params_map and receiver_params_map.
     * @return paramID for this sensor_model.
     */
    sensor_params::id_type paramsID() const {
        return _paramsID;
    }

    /**
     * Gets the minimum active frequency
     * @return double 
     */
    double min_active_freq() const {
        return _min_active_freq;
    }

    /**
     * Gets the maximum active frequency
     * @return double
     */
    double max_active_freq() const {
        return _max_active_freq;
    }

    /**
     * Frequencies of transmitted pulse. Multiple frequencies can be
     * used to compute multiple results at the same time. These are the
     * frequencies at which transmission loss and reverberation are computed.
     * @return frequencies pointer to a seq_vector. 
     */
    seq_vector* frequencies() const {
        return _frequencies.get();
    }

    /**
     * Human readable name for this sensor instance.
     * @return string that contains human readable name of this sensor_model instance.
     */
    const std::string& description() const {
        return _description;
    }

    /**
     * Queries the sensor's ability to support source and/or receiver behaviors.
     * @return mode which is an enumeration of this sensor_model instance.
     */
    xmitRcvModeType mode() const ;

    /**
     * Shared pointer to the the source_params for this sensor.
     * @return A shared pointer to the source_params.
     */
    source_params::reference source() const {
        return _source;
    }

    /**
     * Shared pointer to the the receiver_params for this sensor.
     * @return A shared pointer to the receiver_params.
     */
    receiver_params::reference receiver() const {
        return _receiver;
    }

    /**
     * Location of the sensor in world coordinates.
     * @return the location of this sensor_model instance.
     */
    wposition1 position() const ;

    /**
     * Orientation of the sensor in world coordinates.
     * @return the orientation of this sensor_model instance.
     */
    orientation orient() const ;

    /**
     * Updates the position and orientation of sensor.
     * If the object has changed by more than the threshold amount,
     * this update kicks off a new set of propagation calculations.
     * At the end of those calculations, the eigenverbs and eigenrays
     * are passed onto all sensor listeners.
     * Blocks until update is complete.
     *
     * @param position      Updated position data
     * @param orient        Updated orientation value
     * @param force_update    When true, forces update without checking thresholds.
     */
    void update_sensor(const wposition1& position,
            const orientation& orient, bool force_update = false);

    /**
     * Last set of eigenverbs computed for this sensor.
     * Blocks during updates from the wavefront task.
     * @return shared pointer to and eigenverb_collection.
     */
    eigenverb_collection::reference eigenverbs() const ;

    /**
     * Asynchronous update of eigenrays and eigenverbs data from the wavefront task.
     * Passes this data onto all sensor listeners.
     * Blocks until update is complete.
     * @param eigenrays Shared pointer to an eigenray_collection.
     * @param eigenverbs Shared pointer to an eigenverb_collection.
     */
    virtual void update_wavefront_data(eigenray_collection::reference& eigenrays,
                                        eigenverb_collection::reference& eigenverbs);

    /**
     * Add a sensor_listener to the _sensor_listeners list
     * @param listener  Pointer to a sensor_listener to add
     *                  to the sensor_listeners list.
     */
    void add_sensor_listener(sensor_listener* listener);

    /**
     * Remove a sensor_listener from the _sensor_listeners list.
     * @param listener  Pointer to a sensor_listener to remove
     *                  from the sensor_listeners list.
     */
    void remove_sensor_listener(sensor_listener* listener);

    /**
     * Maximum change in altitude that constitutes new data for
     * eigenverbs and eigenrays be generated.
     */
    static const double alt_threshold ;

    /**
     * Maximum change in latitude that constitutes new data for
     * eigenverbs and eigenrays be generated.
     */
    static const double lat_threshold ;

    /**
     * Maximum change in longitude that constitutes new data for
     * eigenverbs and eigenrays be generated.
     */
    static const double lon_threshold ;

    /**
     * Maximum change in pitch that constitutes new data for
     * eigenverbs and eigenrays be generated.
     */
    static const double pitch_threshold ;

    /**
     * Maximum change in heading that constitutes new data for
     * eigenverbs and eigenrays be generated.
     */
    static const double heading_threshold ;

    /**
     * Maximum change in roll that constitutes new data for
     * eigenverbs and eigenrays be generated.
     */
    static const double roll_threshold ;

private:

    /**
     * Utility to check if new position and orientation have changed enough
     * to require a new WaveQ3D run.
     *
     * @param position      Updated position data
     * @param orient        Updated orientation value
     * @return                 True when thresholds exceeded, requiring a
     *                         rerun of the model for this sensor.
     */
    bool check_thresholds( const wposition1& position,
            const orientation& orient );

    /**
     * Utility to query the current list of sensor listeners for the complements
     * of this sensor. Assumes that these listeners act like sensor_pair objects.
     * @return list of sensor_model pointers that are the complements of the this sensor.
     */
    std::list<const sensor_model*> sensor_targets();

    /**
     * Utility to set the list of target sensorID's from the list of sensors provided.
     * @param list of sensor_model pointers.
     */
    void target_ids(std::list<const sensor_model*>& list);

    /**
     * Utility to builds a list of target positions from the input list of sensors provided.
     * @param list of sensor_model pointers.
     * @return wposition pointer to container of positions of the list of sensors provided.
     */
    const wposition* target_positions(std::list<const sensor_model*>& list) const ;

    /**
     * Utility to run the wave_generator thread task to start the waveq3d model.
     */
    void run_wave_generator();

    /**
     * Utility to set the frequencies band from sensor including
     * min and max active frequencies.
     */
    void frequencies();

    /**
     * Identification used to find this sensor instance in sensor_manager.
     */
    const id_type _sensorID;

    /**
     * Identification used to lookup sensor type data in source_params_map and receiver_params_map.
     */
    const sensor_params::id_type _paramsID;

    /**
     * Minimum active frequency for the sensor.
     */
    double _min_active_freq;

    /**
     *  Maximum active frequency for the sensor.
     */
    double _max_active_freq;

    /**
     * Frequencies of transmitted pulse. Multiple frequencies can be
     * used to compute multiple results at the same time. These are the
     * frequencies at which transmission loss and reverberation are computed.
     */
    unique_ptr<seq_vector> _frequencies;

    /**
     * Enumerated type for the sensor transmit/receiver mode.
     */
    xmitRcvModeType _mode;

    /**
     * Human readable name for this sensor instance.
     */
    const std::string _description;

    /**
     * Shared pointer to the the source_params for this sensor.
     */
    source_params::reference _source;

    /**
     * Shared pointer to the the receiver_params for this sensor.
     */
    receiver_params::reference _receiver;

    /**
     * Location of the sensor in world coordinates.
     */
    wposition1 _position;

    /**
     * Orientation of the sensor in world coordinates.
     */
    orientation _orient;

    /**
     * Flag the designates whether an update requires the creation of
     * new data, because the new position/orientation has changed enough
     * such that the currently cached data for eigenrays and eigenverbs
     * are no longer sufficiently accurate.
     */
    bool _initial_update ;

    /**
     * Mutex that locks sensor during update_sensor.
     */
    mutable read_write_lock _update_sensor_mutex ;

    /**
     * Map containing the target sensorID's and the row offset
     * in _eigenray_collection prior to the staring the wavefront generator.
     */
    std::map<sensor_model::id_type, int> _target_id_map;

    /**
     * Last set of eigenray data computed for this sensor.
     */
    eigenray_collection::reference _eigenray_collection;

    /**
     * Mutex that locks sensor _eigenray_collection access/write
     */
    mutable read_write_lock _eigenrays_mutex ;

    /**
     * Last set of eigenverbs computed for this sensor.
     */
    eigenverb_collection::reference _eigenverb_collection;

    /**
     * Mutex that locks sensor during _eigenverb_collection access/write.
     */
    mutable read_write_lock _eigenverbs_mutex ;

    /**
     * reference to the task that is computing eigenrays and eigenverbs.
     */
    thread_task::reference _wavefront_task;

    /**
     * List containing the references of objects that will be used to
     * update classes that require sensor data.
     * These classes must implement update_fathometer and update_eigenverbs methods.
     */
    std::list<sensor_listener*> _sensor_listeners;

    /**
     * Mutex that locks sensor during add/remove sensor_listeners.
     */
    mutable read_write_lock _sensor_listeners_mutex ;
};

/// @}
}// end of namespace sensors
}  // end of namespace usml
