#ifndef MEASUREMENTS_H
#define MEASUREMENTS_H

// Copyright (c) 2025 Cirque Corp. Restrictions apply. See: www.cirque.com/sw-license

#include "CustomMeas.h"

#define GROUPINFO_LENGTH (1)
#define MEASINFO_LENGTH (2)

// It'd be nice to have this be in some other file set. Measurements.h, Measurements.c
extern CustomMeas::GroupInfo_t GroupInfoArray[GROUPINFO_LENGTH];
extern CustomMeas::MeasInfo_t MeasInfoArray[MEASINFO_LENGTH];

#endif // MEASUREMENTS_H
