Beenode Core version 0.7.3.1
==========================

Release is now available from:

  <https://www.beenode.org/downloads/#wallets>

This is an optional release and only contains changes for testnet. It is not required to update masternodes on mainnet.

Please report bugs using the issue tracker at github:

  <https://github.com/bee-group/beenode/issues>


Upgrading and downgrading
=========================

How to Upgrade
--------------

If you are running an older version, shut it down. Wait until it has completely
shut down (which might take a few minutes for older versions), then run the
installer (on Windows) or just copy over /Applications/Beenode-Qt (on Mac) or
beenoded/beenode-qt (on Linux).

Downgrade warning
-----------------

### Downgrade to a version < 0.7.2.1

Because release 0.7.2.2 included the [per-UTXO fix](release-notes/beenode/release-notes-0.7.2.2.md#per-utxo-fix)
which changed the structure of the internal database, you will have to reindex
the database if you decide to use any pre-0.7.2.2 version.

Wallet forward or backward compatibility was not affected.

### Downgrade to 0.7.3.1/2/3

Downgrading to these versions does not require any additional actions, should be
fully compatible.


Notable changes
===============

Fork/Reset testnet at block 4001
--------------------------------

This release is NOT required on mainnet. It is intended to be deployed on testnet and will cause a fork at block 4001.
The plan is to restart all testing for the v0.13.0.0 upgrade process.

When deployed on testnet, it is required to start with a fresh data directory or call Beenode Core with `-reindex-chainstate`.

0.7.3.1 Change log
===================

See detailed [set of changes](https://github.com/bee-group/beenode/compare/v0.7.2.1...bee-group:v0.7.3.1).

Credits
=======

Thanks to everyone who directly contributed to this release,
as well as everyone who submitted issues and reviewed pull requests.


