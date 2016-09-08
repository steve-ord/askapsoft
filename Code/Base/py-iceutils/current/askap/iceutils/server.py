# Copyright (c) 2012,2016 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA.
#
"""
ZeroC Ice application wrapper
-----------------------------
"""
import sys
import os
import time
import Ice

# pylint: disable-msg=W0611
from askap.slice import FCMService
import askap.interfaces.fcm

from askap.parset import parset_to_dict

from askap import logging

from .monitoringprovider import MonitoringProviderImpl

__all__ = ["Server"]

logger = logging.getLogger(__name__)


class TimeoutError(Exception):
    """An exception raised when call timed out"""
    pass


class Server(object):
    """A class to abstract an ice application (server). It also sets up
    connection to and initialisation from the FCM for the fiven `fcmkey`.
    The configuration can also be given as a command-line arg
    `--config=<cfg_file>` for e.g. functional test set ups.

    :param comm:
    :param fcmkey:
    :param retries:

    """
    def __init__(self, comm, fcmkey='', retries=-1):
        self.parameters = None
        self._comm = comm
        self._adapter = None
        self._mon_adapter = None
        self._services = []
        self._retries = retries
        self.service_key = fcmkey
        self.logger = logger
        self.monitoring = False
        """Enable provision of monitoring (default `False`. This automatically
        creates a :class:`MonitoringProvider` and adapter using the class
        name for adpater naming. e.g. a class named TestServer results
        in  MonitoringService@TestServerAdapter
        """
        self.configurable = True
        """Control configuration via FCM/command-line (default `True`)"""

    def set_retries(self, retries):
        """
        *deprecated*
        """
        self._retries = retries

    def get_config(self):
        """
        Set up FCM configuration from either given file in command-line arg or
        the FCM
        :return:
        """
        if not self.configurable:
            return
        key = "--config="
        for arg in sys.argv:
            if arg.startswith(key):
                k, v = arg.split("=")
                p = os.path.expanduser(os.path.expandvars(v))
                self.parameters = parset_to_dict(open(p).read())
                self.logger.info("Initialized from local config '%s'" % p)
                return
        self._config_from_fcm()

    def _config_from_fcm(self):
        prxy = self._comm.stringToProxy("FCMService@FCMAdapter")
        if not prxy:
            raise RuntimeError("Invalid Proxy for FCMService")
        fcm = self.wait_for_service("FCM",
                                    askap.interfaces.fcm.IFCMServicePrx.checkedCast,
                                    prxy)
        self.parameters = fcm.get(-1, self.service_key)
        self.logger.info("Initialized from fcm")

    def get_parameter(self, key, default=None):
        """
        Get an FCM parameter for the given `key` in the server's namespace

        :param key: the key
        :param default: the default value to return if key now found
        :return: value for the specified key or default

        """
        return self.parameters.get(".".join((self.service_key, key)), default)

    def wait_for_service(self, servicename, callback, *args):
        """
        Try to connect to the registry and establish connection to the given
        Ice service
        :param servicename:
        :param callback:
        :param args:
        :return:
        """
        retval = None
        delay = 5.0
        count = 0
        registry = False
        while not registry:
            try:
                retval = callback(*args)
                registry = True
            except (Ice.ConnectionRefusedException,
                    Ice.NoEndpointException,
                    Ice.NotRegisteredException,
                    Ice.ConnectFailedException,
                    Ice.DNSException) as ex:
                if self._retries > -1 and self._retries == count:
                    msg = "Couldn't connect to {0}: ".format(servicename)
                    self.logger.error(msg+str(ex))
                    raise TimeoutError(msg)
                if count < 10:
                    print >> sys.stderr, "Waiting for", servicename
                if count == 10:
                    print >> sys.stderr, "Repeated 10+ times"
                    self.logger.warn("Waiting for {0}".format(servicename))
                registry = False
                count += 1
                time.sleep(delay)
        if registry:
            self.logger.info("Connected to {0}".format(servicename))
            print >> sys.stderr, servicename, "found"
        return retval

    def setup_services(self):
        adname = self.__class__.__name__ + "Adapter"
        self._adapter = self._comm.createObjectAdapter(adname)

        self.get_config()

        if self.monitoring:
            adname = self.__class__.__name__ + "MonitoringAdapter"
            self._mon_adapter = self._comm.createObjectAdapter(adname)
            self._mon_adapter.add(MonitoringProviderImpl(),
                                  self._comm.stringToIdentity(
                                      "MonitoringService"))
            self.wait_for_service("registry", self._mon_adapter.activate)

        # implement this method in derived class
        self.initialize_services()

        for service in self._services:
            self._adapter.add(service['value'],
                              self._comm.stringToIdentity(service['name']))

        self.wait_for_service("registry", self._adapter.activate)

    def run(self):
        """
        Set up the services and :meth:`wait`.
        """
        self.setup_services()
        self.wait()

    def wait(self):
        """Alias for `communicator.waitForShutdown`"""
        self._comm.waitForShutdown()

    def add_service(self, name, value):
        """
        Add the service implementation `value` under the given `name` (this will
        be the name of the identity.

        :param name: name of the identity to be known under (well known name)
        :param value: the implemnation of the interface to provide

        """
        self._services.append({'name': name, 'value': value})

    # noinspection PyMethodMayBeStatic
    def initialize_services(self):
        """Implement in sub-class with code the set up the server application.
        At a minimum :meth:`add_service` should be called."""
        pass
