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

### Downgrade to a version < 0.12.2.2

Because release 0.12.2.2 included the [per-UTXO fix](release-notes/beenode/release-notes-0.12.2.2.md#per-utxo-fix)
which changed the structure of the internal database, you will have to reindex
the database if you decide to use any pre-0.12.2.2 version.

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


Older releases
==============

Beenode was previously known as Spycoin.

Spycoin tree 0.8.x was a fork of Litecoin tree 0.8, original name was XCoin
which was first released on Jan/18/2014.

Spycoin tree 0.9.x was the open source implementation of masternodes based on
the 0.8.x tree and was first released on Mar/13/2014.

Spycoin tree 0.10.x used to be the closed source implementation of Spysend
which was released open source on Sep/25/2014.

Beenode Core tree 0.11.x was a fork of Bitcoin Core tree 0.9,
Spycoin was rebranded to Beenode.

Beenode Core tree 0.12.0.x was a fork of Bitcoin Core tree 0.10.

Beenode Core tree 0.12.1.x was a fork of Bitcoin Core tree 0.12.

These release are considered obsolete. Old release notes can be found here:

- [v0.7.3.3](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.7.3.3.md) released Sep/19/2018
- [v0.7.3.2](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.7.3.2.md) released Jul/09/2018
- [v0.7.3.1](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.7.3.1.md) released Jul/03/2018
- [v0.12.2.3](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.12.2.3.md) released Jan/12/2018
- [v0.12.2.2](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.12.2.2.md) released Dec/17/2017
- [v0.12.2](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.12.2.md) released Nov/08/2017
- [v0.12.1](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.12.1.md) released Feb/06/2017
- [v0.12.0](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.12.0.md) released Jun/15/2015
- [v0.11.2](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.11.2.md) released Mar/04/2015
- [v0.11.1](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.11.1.md) released Feb/10/2015
- [v0.11.0](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.11.0.md) released Jan/15/2015
- [v0.10.x](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.10.0.md) released Sep/25/2014
- [v0.9.x](https://github.com/bee-group/beenode/blob/master/doc/release-notes/beenode/release-notes-0.9.0.md) released Mar/13/2014
