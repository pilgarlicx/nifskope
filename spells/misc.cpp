#include "../spellbook.h"


class spUpdateArray : public Spell
{
public:
	QString name() const { return Spell::tr( "Update" ); }
	QString page() const { return Spell::tr( "Array" ); }
	QIcon icon() const { return  QIcon( ":/img/update" ); }
	bool instant() const { return true; }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		if ( nif->isArray(index) && nif->evalCondition( index ) )
		{
			//Check if array is of fixed size
			NifItem * item = static_cast<NifItem*>( index.internalPointer() );
			bool static1 = true;
			bool static2 = true;

			if ( item->arr1().isEmpty() == false )
			{
				item->arr1().toInt( &static1 );
			}

			if ( item->arr2().isEmpty() == false )
			{
				item->arr2().toInt( &static2 );
			}

			if ( static1 && static2 )
			{
				//Neither arr1 or arr2 is a variable name
				return false;
			}

			//One of arr1 or arr2 is a variable name so the array is dynamic
			return true;
		}

		return false;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->updateArray( index );
		return index;
	}
};

REGISTER_SPELL( spUpdateArray )

class spUpdateHeader : public Spell
{
public:
	QString name() const { return Spell::tr( "Update" ); }
	QString page() const { return Spell::tr( "Header" ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getHeader() == nif->getBlockOrHeader( index ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->updateHeader();
		return index;
	}
};

REGISTER_SPELL( spUpdateHeader )

class spUpdateFooter : public Spell
{
public:
	QString name() const { return Spell::tr( "Update" ); }
	QString page() const { return Spell::tr( "Footer" ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return ( nif->getFooter() == nif->getBlockOrHeader( index ) );
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		nif->updateFooter();
		return index;
	}
};

REGISTER_SPELL( spUpdateFooter )

class spFollowLink : public Spell
{
public:
	QString name() const { return Spell::tr( "Follow Link" ); }
	bool instant() const { return true; }
	QIcon icon() const { return  QIcon( ":/img/link" ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif->isLink( index ) && nif->getLink( index ) >= 0;
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		QModelIndex idx = nif->getBlock( nif->getLink( index ) );
		if ( idx.isValid() )
			return idx;
		else
			return index;
	}
};

REGISTER_SPELL( spFollowLink )

class spFileOffset : public Spell
{
public:
	QString name() const { return Spell::tr( "File Offset" ); }
	
	bool isApplicable( const NifModel * nif, const QModelIndex & index )
	{
		return nif && index.isValid();
	}
	
	QModelIndex cast( NifModel * nif, const QModelIndex & index )
	{
		int ofs = nif->fileOffset( index );
		qWarning( QString( "estimated file offset is %1 (0x%2)" ).arg( ofs ).arg( ofs, 0, 16 ).toAscii() );
		return index;
	}
};

REGISTER_SPELL( spFileOffset )
