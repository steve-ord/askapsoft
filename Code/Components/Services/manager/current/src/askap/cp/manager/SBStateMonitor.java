/*
 * Copyright (c) 2016 CSIRO - Australia Telescope National Facility (ATNF)
 * 
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * PO Box 76, Epping NSW 1710, Australia
 * atnf-enquiries@csiro.au
 * 
 * This file is part of the ASKAP software distribution.
 * 
 * The ASKAP software distribution is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 * 
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
package askap.cp.manager;

import org.apache.log4j.Logger;

import askap.interfaces.schedblock._ISBStateMonitorDisp;
import askap.interfaces.schedblock.ObsState;

/**
 * Subscriber to the askap.interfaces.schedblock.ISBStateMonitor.changed topic.
 *
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
public class SBStateMonitor extends _ISBStateMonitorDisp {

	private static final Logger logger = Logger.getLogger(CpManager.class.getName());

	/**
	 * SBStateMonitor.changed event handler.
	 *
	 * @param sbid The scheduling block ID.
	 * @param newState The new observation state.
	 * @param updateTime Timestamp of the state change.
	 * @param current The current Ice method invocation information object.
	 */
	@Override
	public void changed(
			long sbid,
			ObsState newState,
			String updateTime,
			Ice.Current current) {
		logger.debug("Schedblock state changed: " + newState.toString());

		if (0 == newState.compareTo(ObsState.PROCESSING)) {
			logger.debug("Schedblock state changed to PROCESSING. Emitting notification");
			// TODO: notify
		}
	}
}
