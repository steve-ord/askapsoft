"""Test CP Manager service interface
"""

import os
import sys
from unittest import skip

from askap.iceutils import CPFuncTestBase, get_service_object

# always import from askap.slice before trying to import any interfaces
from askap.slice import CP
from askap.interfaces.cp import ICPObsServicePrx


class Test(CPFuncTestBase):
    def __init__(self):
        super(Test, self).__init__()
        self.cpclient = None

    def setUp(self):
        # Note that the working directory is 'functests', thus paths are
        # relative to that location.
        os.environ["ICE_CONFIG"] = "config-files/ice.cfg"
        os.environ['TEST_DIR'] = 'test_mock_ingest'
        super(Test, self).setUp()

        try:
            self.cpclient = get_service_object(
                self.ice_session.communicator,
                "CentralProcessorService@CentralProcessorAdapter",
                ICPObsServicePrx)
        except Exception as ex:
            self.shutdown()
            raise

    # @skip('too slow!')
    def test_get_service_version(self):
        #Don't test the full string, as the version changes with SVN revision or tag.
        process_name = self.cpclient.getServiceVersion().split(';')[0]
        assert 'manager' == process_name

    # @skip('too slow!')
    def test_start_abort_wait_observation_sequence(self):
        fbs = self.feedback_service
        fbs.clear_history()

        self.cpclient.startObs(0)
        self.cpclient.abortObs()
        self.cpclient.waitObs(-1)

        print >> sys.stderr, fbs.history.__str__()
        # assert len(fbs.history) == 1
