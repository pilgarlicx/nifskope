#include "blocks.h"

#include <QApplication>
#include <QBuffer>
#include <QClipboard>
#include <QCursor>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QMimeData>
#include <QSettings>


static void addLink( NifModel * nif, QModelIndex iParent, QString array, int link )
{
	QModelIndex iSize = nif->getIndex( iParent, QString( "Num %1" ).arg( array ) );
	QModelIndex iArray = nif->getIndex( iParent, array );
	if ( iSize.isValid() && iArray.isValid() )
	{
		int numlinks = nif->get<int>( iSize );
		nif->set<int>( iSize, numlinks + 1 );
		nif->updateArray( iArray );
		nif->setLink( iArray.child( numlinks, 0 ), link );
	}
}

static void delLink( NifModel * nif, QModelIndex iParent, QString array, int link )
{
	QModelIndex iSize = nif->getIndex( iParent, QString( "Num %1" ).arg( array ) );
	QModelIndex iArray = nif->getIndex( iParent, array );
	QList<qint32> links = nif->getLinkArray( iArray ).toList();
	if ( iSize.isValid() && iArray.isValid() && links.contains( link ) )
	{
		links.removeAll( link );
		nif->set<int>( iSize, links.count() );
		nif->updateArray( iArray );
		nif->setLinkArray( iArray, links.toVector() );
	}
}

static void blockLink( NifModel * nif, const QModelIndex & index, const QModelIndex & iBlock )
{
	if ( nif->isLink( index ) && nif->inherits( iBlock, nif->itemTmplt( index ) ) )
	{
		nif->setLink( index, nif->getBlockNumber( iBlock ) );
	}
	if ( nif->inherits( index, "NiNode" ) && nif->inherits( iBlock, "NiAVObject" ) )
	{
		addLink( nif, index, "Children", nif->getBlockNumber( iBlock ) );
	}
	else if ( nif->inherits( index, "NiAVObject" ) && nif->inherits( iBlock, "NiProperty" ) )
	{
		addLink( nif, index, "Properties", nif->getBlockNumber( iBlock ) );
	}
	else if ( nif->inherits( index, "NiAVObject" ) && nif->inherits( iBlock, "NiExtraData" ) )
	{
		addLink( nif, index, "Extra Data List", nif->getBlockNumber( iBlock ) );
	}
}

class spInsertBlock : public Spell
{
public:
	QString name() const { return "Insert"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( ! index.isValid() || ! index.parent().isValid() );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		
		QMap< QString, QMenu *> map;
		foreach ( QString id, ids )
		{
			QString x( "Other" );
			
			if ( id.startsWith( "Ni" ) )
				x = QString("Ni&") + id.mid( 2, 1 ) + "...";
			if ( id.startsWith( "bhk" ) || id.startsWith( "hk" ) )
				x = "Havok";
			if ( id.startsWith( "BS" ) || id == "AvoidNode" || id == "RootCollisionNode" )
				x = "Bethesda";
			if ( id.startsWith( "Fx" ) )
				x = "Firaxis";
			
			if ( ! map.contains( x ) )
				map[ x ] = new QMenu( x );
			map[ x ]->addAction( id );
		}

		QMenu menu;
		foreach ( QMenu * m, map )
			menu.addMenu( m );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
			return nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
		else
			return index;
	}
};

REGISTER_SPELL( spInsertBlock )


class spAttachProperty : public Spell
{
public:
	QString name() const { return "Attach Property"; }
	QString page() const { return "Node"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->itemType( index ) == "NiBlock" && nif->inherits( index, "NiAVObject" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiProperty" ) )
				menu.addAction( id );
		if ( menu.actions().isEmpty() )
			return index;
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iProperty = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Properties", nif->getBlockNumber( iProperty ) );
			return iProperty;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachProperty )


class spAttachNode : public Spell
{
public:
	QString name() const { return "Attach Node"; }
	QString page() const { return "Node"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiNode" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiAVObject" ) && ! nif->inherits( id, "NiLight" ) )
				menu.addAction( id );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iNode = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Children", nif->getBlockNumber( iNode ) );
			return iNode;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachNode )


class spAttachLight : public Spell
{
public:
	QString name() const { return "Attach Light"; }
	QString page() const { return "Node"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiNode" );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiLight" ) )
				menu.addAction( id );
		
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iLight = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Children", nif->getBlockNumber( iLight ) );
			addLink( nif, iParent, "Effects", nif->getBlockNumber( iLight ) );
			return iLight;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachLight )


class spAttachExtraData : public Spell
{
public:
	QString name() const { return "Attach Extra Data"; }
	QString page() const { return "Node"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->inherits( index, "NiObjectNET" ) && nif->checkVersion( 0x0a000100, 0 );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QMenu menu;
		QStringList ids = nif->allNiBlocks();
		ids.sort();
		foreach ( QString id, ids )
			if ( nif->inherits( id, "NiExtraData" ) )
				menu.addAction( id );
		QAction * act = menu.exec( QCursor::pos() );
		if ( act )
		{
			QPersistentModelIndex iParent = index;
			QModelIndex iExtra = nif->insertNiBlock( act->text(), nif->getBlockNumber( index ) + 1 );
			addLink( nif, iParent, "Extra Data List", nif->getBlockNumber( iExtra ) );
			return iExtra;
		}
		else
			return index;
	}
};

REGISTER_SPELL( spAttachExtraData )


class spRemoveBlock : public Spell
{
public:
	QString name() const { return "Remove"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) >= 0;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->removeNiBlock( nif->getBlockNumber( index ) );
		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBlock )


class spCopyBlock : public Spell
{
public:
	QString name() const { return "Copy"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QByteArray data;
		QBuffer buffer( & data );
		if ( buffer.open( QIODevice::WriteOnly ) && nif->save( buffer, index ) )
		{
			QMimeData * mime = new QMimeData;
			mime->setData( QString( "nifskope/niblock/%1/%2" ).arg( nif->itemName( index ) ).arg( nif->getVersion() ), data );
			QApplication::clipboard()->setMimeData( mime );
		}
		return index;
	}
};

REGISTER_SPELL( spCopyBlock )


class spPasteBlock : public Spell
{
public:
	QString name() const { return "Paste"; }
	QString page() const { return "Block"; }
	
	QString acceptFormat( const QString & format, const NifModel * nif )
	{
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "niblock" && nif->isNiBlock( split.value( 2 ) ) )
			return split.value( 3 );
		return QString();
	}
	
	QString blockType( const QString & format )
	{
		return format.split( "/" ).value( 2 );
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( ! acceptFormat( form, nif ).isEmpty() )
					return true;
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				QString version = acceptFormat( form, nif );
				if ( ! version.isEmpty() && ( version == nif->getVersion() || QMessageBox::question( 0, "Paste Block", QString( "Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable..." ).arg( nif->getVersion() ).arg( version ), "Continue", "Cancel" ) == 0 ))
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						QModelIndex block = nif->insertNiBlock( blockType( form ), nif->getBlockCount() );
						nif->load( buffer, block );
						blockLink( nif, index, block );
						return block;
					}
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteBlock )


class spPasteOverBlock : public Spell
{
public:
	QString name() const { return "Paste Over"; }
	QString page() const { return "Block"; }
	
	QString acceptFormat( const QString & format, const NifModel * nif, const QModelIndex & block )
	{
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "niblock" && nif->isNiBlock( block, split.value( 2 ) ) )
			return split.value( 3 );
		return QString();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( ! acceptFormat( form, nif, index ).isEmpty() )
					return true;
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				QString version = acceptFormat( form, nif, index );
				if ( ! version.isEmpty() && ( version == nif->getVersion() || QMessageBox::question( 0, "Paste Over", QString( "Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable..." ).arg( nif->getVersion() ).arg( version ), "Continue", "Cancel" ) == 0 ))
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						nif->load( buffer, index );
						return index;
					}
				}
			}
		}
		return QModelIndex();
	}
};

REGISTER_SPELL( spPasteOverBlock )


class spCopyBranch : public Spell
{
public:
	QString name() const { return "Copy Branch"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QList<qint32> blocks;
		populateBlocks( blocks, nif, nif->getBlockNumber( index ) );
		
		QMap<qint32,qint32> blockMap;
		for ( int b = 0; b < blocks.count(); b++ )
			blockMap.insert( blocks[b], b );
		
		QMap<qint32,QString> parentMap;
		foreach ( qint32 block, blocks )
		{
			foreach ( qint32 link, nif->getParentLinks( block ) )
			{
				if ( ! blocks.contains( link ) && ! parentMap.contains( link ) )
				{
					QModelIndex iParent = nif->getBlock( link );
					if ( iParent.isValid() )
					{
						QString name = nif->get<QString>( iParent, "Name" );
						if ( ! name.isEmpty() )
						{
							parentMap.insert( link, nif->itemName( iParent ) + "|" + name );
							continue;
						}
					}
					qWarning() << "failed to map parent link" << link << "for block" << block << nif->itemName( nif->getBlock( block ) );
					return index;
				}
			}
		}
		
		QByteArray data;
		QBuffer buffer( & data );
		if ( buffer.open( QIODevice::WriteOnly ) )
		{
			QDataStream ds( & buffer );
			ds << blocks.count();
			ds << blockMap;
			ds << parentMap;
			foreach ( qint32 block, blocks )
			{
				ds << nif->itemName( nif->getBlock( block ) );
				if ( ! nif->save( buffer, nif->getBlock( block ) ) )
				{
					qWarning() << "failed to save block" << block << nif->itemName( nif->getBlock( block ) );
					return index;
				}
			}
			QMimeData * mime = new QMimeData;
			mime->setData( QString( "nifskope/nibranch/%1" ).arg( nif->getVersion() ), data );
			QApplication::clipboard()->setMimeData( mime );
		}
		return index;
	}
	
	void populateBlocks( QList<qint32> & blocks, NifModel * nif, qint32 block )
	{
		if ( ! blocks.contains( block ) ) blocks.append( block );
		foreach ( qint32 link, nif->getChildLinks( block ) )
			populateBlocks( blocks, nif, link );
	}
};

REGISTER_SPELL( spCopyBranch )


class spPasteBranch : public Spell
{
public:
	QString name() const { return "Paste Branch"; }
	QString page() const { return "Block"; }
	
	QString acceptFormat( const QString & format, const NifModel * nif )
	{
		QStringList split = format.split( "/" );
		if ( split.value( 0 ) == "nifskope" && split.value( 1 ) == "nibranch" )
			return split.value( 2 );
		else
			return QString();
	}
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		if ( index.isValid() && ! nif->isNiBlock( index ) && ! nif->isLink( index ) )
			return false;
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
			foreach ( QString form, mime->formats() )
				if ( nif->isVersionSupported( nif->version2number( acceptFormat( form, nif ) ) ) )
					return true;
		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		const QMimeData * mime = QApplication::clipboard()->mimeData();
		if ( mime )
		{
			foreach ( QString form, mime->formats() )
			{
				QString v = acceptFormat( form, nif );
				if ( ! v.isEmpty() && ( v == nif->getVersion() || QMessageBox::question( 0, "Paste Branch", QString( "Nif versions differ!<br><br>Current File Version: %1<br>Clipboard Data Version: %2<br><br>The results will be unpredictable..." ).arg( nif->getVersion() ).arg( v ), "Continue", "Cancel" ) == 0 ) )
				{
					QByteArray data = mime->data( form );
					QBuffer buffer( & data );
					if ( buffer.open( QIODevice::ReadOnly ) )
					{
						QDataStream ds( & buffer );
						
						int count;
						ds >> count;
						
						QMap<qint32,qint32> blockMap;
						ds >> blockMap;
						QMutableMapIterator<qint32,qint32> ibm( blockMap );
						while ( ibm.hasNext() )
						{
							ibm.next();
							ibm.value() += nif->getBlockCount();
						}
						
						QMap<qint32,QString> parentMap;
						ds >> parentMap;
						
						QMapIterator<qint32, QString> ipm( parentMap );
						while ( ipm.hasNext() )
						{
							ipm.next();
							qint32 block = getBlockByName( nif, ipm.value() );
							if ( block >= 0 )
							{
								blockMap.insert( ipm.key(), block );
							}
							else
							{
								qWarning() << "failed to map parent link" << ipm.value();
								return index;
							}
						}
						
						QModelIndex iRoot;
						
						for ( int c = 0; c < count; c++ )
						{
							QString type;
							ds >> type;
							
							QModelIndex block = nif->insertNiBlock( type, -1 );
							if ( ! nif->loadAndMapLinks( buffer, block, blockMap ) )
								return index;
							if ( c == 0 )
								iRoot = block;
						}
						
						blockLink( nif, index, iRoot );
						
						return iRoot;
					}
				}
			}
		}
		return QModelIndex();
	}
	
	qint32 getBlockByName( NifModel * nif, const QString & tn )
	{
		QStringList ls = tn.split( "|" );
		QString type = ls.value( 0 );
		QString name = ls.value( 1 );
		if ( type.isEmpty() || name.isEmpty() )
			return -1;
		for ( int b = 0; b < nif->getBlockCount(); b++ )
		{
			QModelIndex iBlock = nif->getBlock( b );
			if ( nif->itemName( iBlock ) == type && nif->get<QString>( iBlock, "Name" ) == name )
				return b;
		}
		return -1;
	}
};

REGISTER_SPELL( spPasteBranch )


	static void removeChildren( NifModel * nif, const QPersistentModelIndex & iBlock )
	{
		QList<QPersistentModelIndex> iChildren;
		foreach ( quint32 link, nif->getChildLinks( nif->getBlockNumber( iBlock ) ) )
			iChildren.append( nif->getBlock( link ) );
		
		foreach ( QPersistentModelIndex iChild, iChildren )
			if ( iChild.isValid() && nif->getBlockNumber( iBlock ) == nif->getParent( nif->getBlockNumber( iChild ) ) )
				removeChildren( nif, iChild );
		
		foreach ( QPersistentModelIndex iChild, iChildren )
			if ( iChild.isValid() && nif->getBlockNumber( iBlock ) == nif->getParent( nif->getBlockNumber( iChild ) ) )
				nif->removeNiBlock( nif->getBlockNumber( iChild ) );
	}

bool spRemoveBranch::isApplicable( const NifModel * nif, const QModelIndex & iBlock )
{
	int ix = nif->getBlockNumber( iBlock );
	return ( nif->isNiBlock( iBlock ) && ix >= 0 && ( nif->getRootLinks().contains( ix ) || nif->getParent( ix ) >= 0 ) );
}

QModelIndex spRemoveBranch::cast( NifModel * nif, const QModelIndex & index )
{
	QPersistentModelIndex iBlock = index;
	removeChildren( nif, iBlock );
	nif->removeNiBlock( nif->getBlockNumber( iBlock ) );
	return QModelIndex();
}

REGISTER_SPELL( spRemoveBranch )


class spFlattenBranch : public Spell
{
public:
	QString name() const { return "Flatten Branch"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		QModelIndex iParent = nif->getBlock( nif->getParent( nif->getBlockNumber( index ) ), "NiNode" );
		return nif->inherits( index, "NiNode" ) && iParent.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iNode )
	{
		QModelIndex iParent = nif->getBlock( nif->getParent( nif->getBlockNumber( iNode ) ), "NiNode" );
		doNode( nif, iNode, iParent, Transform() );
		return iNode;
	}
	
	void doNode( NifModel * nif, const QModelIndex & iNode, const QModelIndex & iParent, Transform tp )
	{
		if ( ! nif->inherits( iNode, "NiNode" ) )
			return;
		
		Transform t = tp * Transform( nif, iNode );
		
		QList<qint32> links;
		
		foreach ( qint32 l, nif->getLinkArray( iNode, "Children" ) )
		{
			QModelIndex iChild = nif->getBlock( l );
			if ( nif->getParent( nif->getBlockNumber( iChild ) ) == nif->getBlockNumber( iNode ) )
			{
				Transform tc = t * Transform( nif, iChild );
				tc.writeBack( nif, iChild );
				addLink( nif, iParent, "Children", l );
				delLink( nif, iNode, "Children", l );
				links.append( l );
			}
		}
		
		foreach ( qint32 l, links )
		{
			doNode( nif, nif->getBlock( l, "NiNode" ), iParent, tp );
		}
	}
};

REGISTER_SPELL( spFlattenBranch )


class spMoveBlockUp : public Spell
{
public:
	QString name() const { return "Move Up"; }
	QString page() const { return "Block"; }
	QKeySequence hotkey() const { return QKeySequence( Qt::CTRL + Qt::Key_Up ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) > 0;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock )
	{
		int ix = nif->getBlockNumber( iBlock );
		nif->moveNiBlock( ix, ix - 1 );
		return nif->getBlock( ix - 1 );
	}
};

REGISTER_SPELL( spMoveBlockUp )


class spMoveBlockDown : public Spell
{
public:
	QString name() const { return "Move Down"; }
	QString page() const { return "Block"; }
	QKeySequence hotkey() const { return QKeySequence( Qt::CTRL + Qt::Key_Down ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isNiBlock( index ) && nif->getBlockNumber( index ) < nif->getBlockCount() - 1;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & iBlock )
	{
		int ix = nif->getBlockNumber( iBlock );
		nif->moveNiBlock( ix, ix + 1 );
		return nif->getBlock( ix + 1 );
	}
};

REGISTER_SPELL( spMoveBlockDown )


class spRemoveBlocksById : public Spell
{
public:
	QString name() const { return "Remove By Id"; }
	QString page() const { return "Block"; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ! index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & )
	{
		QSettings settings( "NifTools", "NifSkope" );
		settings.beginGroup( "spells" );
		settings.beginGroup( page() );
		settings.beginGroup( name() );
		
		bool ok = true;
		QString match = QInputDialog::getText( 0, "Remove Blocks by Id", "Enter a regular expression:", QLineEdit::Normal,
			settings.value( "match expression", "^BS|^NiBS|^bhk|^hk" ).toString(), & ok );
		
		if ( ! ok )
			return QModelIndex();
		
		settings.setValue( "match expression", match );
		
		QRegExp exp( match );
		
		int n = 0;
		while ( n < nif->getBlockCount() )
		{
			QModelIndex iBlock = nif->getBlock( n );
			if ( nif->itemName( iBlock ).indexOf( exp ) >= 0 )
				nif->removeNiBlock( n );
			else
				n++;
		}
		
		return QModelIndex();
	}
};

REGISTER_SPELL( spRemoveBlocksById )

