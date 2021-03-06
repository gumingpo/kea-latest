// Copyright (C) 2015,2017 Internet Systems Consortium, Inc. ("ISC")
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef STATSMGR_H
#define STATSMGR_H

#include <stats/observation.h>
#include <stats/context.h>
#include <boost/noncopyable.hpp>

#include <map>
#include <string>
#include <vector>
#include <sstream>

namespace isc {
namespace stats {

/// @brief Statistics Manager class
///
/// StatsMgr is a singleton class that represents a subsystem that manages
/// collection, storage and reporting of various types of statistics.
/// It is also the intended API for both core code and hooks.
///
/// As of May 2015, Tomek ran performance benchmarks (see unit-tests in
/// stats_mgr_unittest.cc with performance in their names) and it seems
/// the code is able to register ~2.5-3 million observations per second, even
/// with 1000 different statistics recorded. That seems sufficient for now,
/// so there is no immediate need to develop any multi-threading solutions
/// for now. However, should this decision be revised in the future, the
/// best place for it would to be modify @ref addObservation method here.
/// It's the common code point that all new observations must pass through.
/// One possible way to enable multi-threading would be to run a separate
/// thread handling collection. The main thread would call @ref addValue and
/// @ref setValue methods that would end up calling @ref addObservation.
/// That method would pass the data to separate thread to be collected and
/// would immediately return. Further processing would be mostly as it
/// is today, except happening in a separate thread. One unsolved issue in
/// this approach is how to extract data, but that will remain unsolvable
/// until we get the control socket implementation.
///
/// Statistics Manager does not use logging by design. The reasons are:
/// - performance impact (logging every observation would degrade performance
///   significantly. While it's possible to log on sufficiently high debug
///   level, such a log would be not that useful)
/// - dependency (statistics are intended to be a lightweight library, adding
///   dependency on libkea-log, which has its own dependencies, including
///   external log4cplus, is against 'lightweight' design)
/// - if logging of specific statistics is warranted, it is recommended to
///   add log entries in the code that calls StatsMgr.
/// - enabling logging in StatsMgr does not offer fine tuning. It would be
///   either all or nothing. Adding logging entries only when necessary
///   in the code that uses StatsMgr gives better granularity.
///
/// If this decision is revisited in the future, the most universal places
/// for adding logging have been marked in @ref addValueInternal and
/// @ref setValueInternal.
class StatsMgr : public boost::noncopyable {
 public:

    /// @brief Statistics Manager accessor method.
    static StatsMgr& instance();

    /// @defgroup producer_methods Methods are used by data producers.
    ///
    /// @brief The following methods are used by data producers:
    ///
    /// @{

    /// @brief Records absolute integer observation.
    ///
    /// @param name name of the observation
    /// @param value integer value observed
    /// @throw InvalidStatType if statistic is not integer
    void setValue(const std::string& name, const int64_t value);

    /// @brief Records absolute floating point observation.
    ///
    /// @param name name of the observation
    /// @param value floating point value observed
    /// @throw InvalidStatType if statistic is not fp
    void setValue(const std::string& name, const double value);

    /// @brief Records absolute duration observation.
    ///
    /// @param name name of the observation
    /// @param value duration value observed
    /// @throw InvalidStatType if statistic is not time duration
    void setValue(const std::string& name, const StatsDuration& value);

    /// @brief Records absolute string observation.
    ///
    /// @param name name of the observation
    /// @param value string value observed
    /// @throw InvalidStatType if statistic is not a string
    void setValue(const std::string& name, const std::string& value);

    /// @brief Records incremental integer observation.
    ///
    /// @param name name of the observation
    /// @param value integer value observed
    /// @throw InvalidStatType if statistic is not integer
    void addValue(const std::string& name, const int64_t value);

    /// @brief Records incremental floating point observation.
    ///
    /// @param name name of the observation
    /// @param value floating point value observed
    /// @throw InvalidStatType if statistic is not fp
    void addValue(const std::string& name, const double value);

    /// @brief Records incremental duration observation.
    ///
    /// @param name name of the observation
    /// @param value duration value observed
    /// @throw InvalidStatType if statistic is not time duration
    void addValue(const std::string& name, const StatsDuration& value);

    /// @brief Records incremental string observation.
    ///
    /// @param name name of the observation
    /// @param value string value observed
    /// @throw InvalidStatType if statistic is not a string
    void addValue(const std::string& name, const std::string& value);

    /// @brief Determines maximum age of samples.
    ///
    /// Specifies that statistic name should be stored not as a single value,
    /// but rather as a set of values. duration determines the timespan.
    /// Samples older than duration will be discarded. This is time-constrained
    /// approach. For sample count constrained approach, see @ref
    /// setMaxSampleCount() below.
    ///
    /// @todo: Not implemented.
    ///
    /// Example: to set a statistic to keep observations for the last 5 minutes,
    /// call setMaxSampleAge("incoming-packets", time_duration(0,5,0,0));
    /// to revert statistic to a single value, call:
    /// setMaxSampleAge("incoming-packets" time_duration(0,0,0,0))
    void setMaxSampleAge(const std::string& name, const StatsDuration& duration);

    /// @brief Determines how many samples of a given statistic should be kept.
    ///
    /// Specifies that statistic name should be stored not as single value, but
    /// rather as a set of values. In this form, at most max_samples will be kept.
    /// When adding max_samples+1 sample, the oldest sample will be discarded.
    ///
    /// @todo: Not implemented.
    ///
    /// Example:
    /// To set a statistic to keep the last 100 observations, call:
    /// setMaxSampleCount("incoming-packets", 100);
    void setMaxSampleCount(const std::string& name, uint32_t max_samples);

    /// @}

    /// @defgroup consumer_methods Methods are used by data consumers.
    ///
    /// @brief The following methods are used by data consumers:
    ///
    /// @{

    /// @brief Resets specified statistic.
    ///
    /// This is a convenience function and is equivalent to setValue(name,
    /// neutral_value), where neutral_value is 0, 0.0 or "".
    /// @param name name of the statistic to be reset.
    /// @return true if successful, false if there's no such statistic
    bool reset(const std::string& name);

    /// @brief Removes specified statistic.
    ///
    /// @param name name of the statistic to be removed.
    /// @return true if successful, false if there's no such statistic
    bool del(const std::string& name);

    /// @brief Resets all collected statistics back to zero.
    void resetAll();

    /// @brief Removes all collected statistics.
    void removeAll();

    /// @brief Returns number of available statistics.
    ///
    /// @return number of recorded statistics.
    size_t count() const;

    /// @brief Returns a single statistic as a JSON structure.
    ///
    /// @return JSON structures representing a single statistic
    isc::data::ConstElementPtr get(const std::string& name) const;

    /// @brief Returns all statistics as a JSON structure.
    ///
    /// @return JSON structures representing all statistics
    isc::data::ConstElementPtr getAll() const;

    /// @}

    /// @brief Returns an observation.
    ///
    /// Used in testing only. Production code should use @ref get() method.
    /// @param name name of the statistic
    /// @return Pointer to the Observation object
    ObservationPtr getObservation(const std::string& name) const;

    /// @brief Generates statistic name in a given context
    ///
    /// Example:
    /// @code
    /// generateName("subnet", 123, "received-packets");
    /// @endcode
    /// will return subnet[123].received-packets. Any printable type
    /// can be used as index.
    ///
    /// @tparam Type any type that can be used to index contexts
    /// @param context name of the context (e.g. 'subnet')
    /// @param index value used for indexing contexts (e.g. subnet_id)
    /// @param stat_name name of the statistic
    /// @return returns full statistic name in form context[index].stat_name
    template<typename Type>
    static std::string generateName(const std::string& context, Type index,
                             const std::string& stat_name) {
        std::stringstream name;
        name << context << "[" << index << "]." << stat_name;
        return (name.str());
    }

    /// @defgroup command_methods Methods are used to handle commands.
    ///
    /// @brief The following methods are used to handle commands:
    ///
    /// @{

    /// @brief Handles statistic-get command
    ///
    /// This method handles statistic-get command, which returns value
    /// of a given statistic). It expects one parameter stored in params map:
    /// name: name-of-the-statistic
    ///
    /// Example params structure:
    /// {
    ///     "name": "packets-received"
    /// }
    ///
    /// @param name name of the command (ignored, should be "statistic-get")
    /// @param params structure containing a map that contains "name"
    /// @return answer containing details of specified statistic
    static isc::data::ConstElementPtr
    statisticGetHandler(const std::string& name,
                        const isc::data::ConstElementPtr& params);

    /// @brief Handles statistic-reset command
    ///
    /// This method handles statistic-reset command, which resets value
    /// of a given statistic. It expects one parameter stored in params map:
    /// name: name-of-the-statistic
    ///
    /// Example params structure:
    /// {
    ///     "name": "packets-received"
    /// }
    ///
    /// @param name name of the command (ignored, should be "statistic-reset")
    /// @param params structure containing a map that contains "name"
    /// @return answer containing confirmation
    static isc::data::ConstElementPtr
    statisticResetHandler(const std::string& name,
                          const isc::data::ConstElementPtr& params);

    /// @brief Handles statistic-remove command
    ///
    /// This method handles statistic-reset command, which removes a given
    /// statistic completely. It expects one parameter stored in params map:
    /// name: name-of-the-statistic
    ///
    /// Example params structure:
    /// {
    ///     "name": "packets-received"
    /// }
    ///
    /// @param name name of the command (ignored, should be "statistic-remove")
    /// @param params structure containing a map that contains "name" element
    /// @return answer containing confirmation
    static isc::data::ConstElementPtr
    statisticRemoveHandler(const std::string& name,
                           const isc::data::ConstElementPtr& params);

    /// @brief Handles statistic-get-all command
    ///
    /// This method handles statistic-get-all command, which returns values
    /// of all statistics. Params parameter is ignored.
    ///
    /// @param name name of the command (ignored, should be "statistic-get-all")
    /// @param params ignored
    /// @return answer containing values of all statistic
    static isc::data::ConstElementPtr
    statisticGetAllHandler(const std::string& name,
                           const isc::data::ConstElementPtr& params);

    /// @brief Handles statistic-reset-all command
    ///
    /// This method handles statistic-reset-all command, which sets values of
    /// all statistics back to zero. Params parameter is ignored.
    ///
    /// @param name name of the command (ignored, should be "statistic-reset-all")
    /// @param params ignored
    /// @return answer confirming success of this operation
    static isc::data::ConstElementPtr
    statisticResetAllHandler(const std::string& name,
                             const isc::data::ConstElementPtr& params);

    /// @brief Handles statistic-remove-all command
    ///
    /// This method handles statistic-remove-all command, which removes all
    /// statistics. Params parameter is ignored.
    ///
    /// @param name name of the command (ignored, should be "statistic-remove-all")
    /// @param params ignored
    /// @return answer confirming success of this operation
    static isc::data::ConstElementPtr
    statisticRemoveAllHandler(const std::string& name,
                              const isc::data::ConstElementPtr& params);

    /// @}

 private:

    /// @brief Private constructor.
    /// StatsMgr is a singleton. It should be accessed using @ref instance
    /// method.
    StatsMgr();

    /// @public

    /// @brief Sets a given statistic to specified value (internal version).
    ///
    /// This template method sets statistic identified by name to a value
    /// specified by value. This internal method is used by public @ref setValue
    /// methods.
    ///
    /// @tparam DataType one of int64_t, double, StatsDuration or string
    /// @param name name of the statistic
    /// @param value specified statistic will be set to this value
    /// @throw InvalidStatType is statistic exists and has a different type.
    template<typename DataType>
    void setValueInternal(const std::string& name, DataType value) {

        // If we want to log each observation, here would be the best place for it.
        ObservationPtr stat = getObservation(name);
        if (stat) {
            stat->setValue(value);
        } else {
            stat.reset(new Observation(name, value));
            addObservation(stat);
        }
    }

    /// @public

    /// @brief Adds specified value to a given statistic (internal version).
    ///
    /// This template method adds specified value to a given statistic (identified
    /// by name to a value). This internal method is used by public @ref setValue
    /// methods.
    ///
    /// @tparam DataType one of int64_t, double, StatsDuration or string
    /// @param name name of the statistic
    /// @param value specified statistic will be set to this value
    /// @throw InvalidStatType is statistic exists and has a different type.
    template<typename DataType>
    void addValueInternal(const std::string& name, DataType value) {

        // If we want to log each observation, here would be the best place for it.
        ObservationPtr existing = getObservation(name);
        if (!existing) {
            // We tried to add to a non-existing statistic. We can recover from
            // that. Simply add the new incremental value as a new statistic and
            // we're done.
            setValue(name, value);
            return;
        } else {
            // Let's hope it is of correct type. If not, the underlying
            // addValue() method will throw.
            existing->addValue(value);
        }
    }

    /// @public

    /// @brief Adds a new observation.
    ///
    /// That's an utility method used by public @ref setValue() and
    /// @ref addValue() methods.
    /// @param stat observation
    void addObservation(const ObservationPtr& stat);

    /// @private

    /// @brief Tries to delete an observation.
    ///
    /// @param name of the statistic to be deleted
    /// @return true if deleted, false if not found
    bool deleteObservation(const std::string& name);

    /// @brief Utility method that attempts to extract statistic name
    ///
    /// This method attempts to extract statistic name from the params
    /// structure. It is expected to be a map that contains 'name' element,
    /// that is of type string. If present as expected, statistic name
    /// set and true is returned. If missing or is of incorrect type, the reason
    /// is specified in reason parameter and false is returned.
    ///
    /// @param params parameters structure received in command
    /// @param name [out] name of the statistic (if no error detected)
    /// @param reason [out] failure reason (if error is detected)
    /// @return true (if everything is ok), false otherwise
    static bool getStatName(const isc::data::ConstElementPtr& params,
                            std::string& name,
                            std::string& reason);

    // This is a global context. All statistics will initially be stored here.
    StatContextPtr global_;
};

};
};

#endif // STATS_MGR
