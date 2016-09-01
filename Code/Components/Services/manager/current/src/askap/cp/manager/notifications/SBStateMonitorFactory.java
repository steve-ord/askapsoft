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
package askap.cp.manager.notifications;

import askap.util.ParameterSet;

/**
 *
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
public final class SBStateMonitorFactory {

	/**
	 * Factory method for SBStateMonitor.
	 * 
	 * @param type: Identifies the type of state monitor to create.
	 * @param config: The parameter set.
	 * @return SBStateMonitor
	 */
	public static SBStateMonitor getInstance(String type, ParameterSet config) {
		switch (type) {
			case "test":
				return new TestSBStateChangedMonitor(config);
			case "jira":
				return new JiraSBStateChangedMonitor(config);
			default:
				throw new IllegalArgumentException(type);
		}
	}
}
