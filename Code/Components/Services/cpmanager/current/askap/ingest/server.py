import Ice, IceStorm

from askap.iceutils import Server

from . import logger

from ObsService import CPObsServiceImp
from svcclients import IngestMonitor
from JIRAStateChangeMonitor import JIRAStateChangeMonitor


class IngestManager(Server):
    def __init__(self, comm):
        Server.__init__(self, comm, fcmkey='cp.ingest')
        self._monitor = None
        self.logger = logger
        self.monitoring = True

    def initialize_services(self):
        logger.debug('initialize_services')

        cp_obsServer = CPObsServiceImp(self.fcm)
        self.add_service("CentralProcessorService", cp_obsServer)

        ingest_monitor = IngestMonitor(self._comm, self.parameters, cp_obsServer)
        ingest_monitor.start()

        topicname = "sbstatechange"
        prx = self._comm.stringToProxy(
            'IceStorm/TopicManager@IceStorm.TopicManager')

        logger.debug('Connected to topic manager')

        manager = self.wait_for_service("IceStorm",
                                        IceStorm.TopicManagerPrx.checkedCast,
                                        prx)

        try:
            topic = manager.retrieve(topicname)
        except IceStorm.NoSuchTopic:
            try:
                topic = manager.create(topicname)
            except IceStorm.TopicExists:
                topic = manager.retrieve(topicname)

        try:
            adapter = self._comm.createObjectAdapterWithEndpoints("SBStateMonitorAdapter", "tcp")
            subscriber = adapter.addWithUUID(JIRAStateChangeMonitor(self.parameters)).ice_oneway()
            adapter.activate()
            topic.subscribeAndGetPublisher({}, subscriber)

        except:
            raise RuntimeError("ICE adapter initialisation failed")


        logger.debug('initialize_services finished')
