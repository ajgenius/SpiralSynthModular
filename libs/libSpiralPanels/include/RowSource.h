// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* Rows over a flat source's numbered fields. A device that keeps eight
 * samples offers LOOP0..LOOP7, NOTE0..NOTE7 and so on as fields of its one
 * row; this presents them as eight rows with the fields LOOP and NOTE, so a
 * control binds to "the loop setting of the sample on show" and the cursor
 * says which sample that is. The data stays where it was: a row's field is
 * read from and written to the flat source's field of that number.
 *
 * The cursor can be this source's own, or a field of the flat source: a
 * selector bound to that field then moves every control bound here at once,
 * and a move made here writes the field. The number of rows can be fixed,
 * read from a field (a channel count), or found by counting the numbered
 * fields that exist.
 *
 * Some data a device hands over whole instead: a sequencer's lines come as
 * one table, not as a setting for each cell. The rows can be over such a
 * table too. Its columns are then the fields, named by their number, and a
 * cell is changed by a command to the device, not by writing here.
 */
#ifndef SPIRAL_DATA_ROW_SOURCE_H
#define SPIRAL_DATA_ROW_SOURCE_H
#include "DataSource.h"

namespace Spiral
{
	namespace Data
	{
		/* A table handed over whole: the cells of its rows one after another,
		 * as many to a row as the row source was told. False when there is
		 * nothing to read yet. */
		class TableFeed
		{
		public:
			virtual ~TableFeed() {}
			virtual bool Cells(std::vector<double> &out) const = 0;
		};

		class RowSource: public DataSource, public DataObserver
		{
		public:
			/* Over a source that outlives this one or says when it dies. Row 0's
			 * fields carry the number `first`: 0 for LOOP0, 1 for a device that
			 * counts from one. */
			explicit RowSource(DataSource *flat, long first = 0);
			virtual ~RowSource();

			/* A field of the rows, by the name its numbered fields share. Its
			 * type, label and whether it can be written are row 0's. Fails when
			 * the flat source has no such field for row 0. */
			int AddField(const std::string &base);

			/* The rows are a table's instead, a cell to each column named, and
			 * the fields its columns: numbers, read only. Given a count in
			 * place of names, the columns are named "0", "1" and on. The feed
			 * is this source's to delete. The count and the cursor are still
			 * fields of the flat source, or fixed. */
			void TableFrom(TableFeed *feed, const std::vector<std::string> &columns);
			void TableFrom(TableFeed *feed, int columns);

			/* How many rows: this many, or whatever a field of the flat source
			 * holds, limited to the rows whose fields exist. With neither, every
			 * row whose fields all exist. */
			void SetCount(long rows);
			bool CountFrom(const std::string &field);

			/* The cursor is a field of the flat source holding a row number. */
			bool PositionFrom(const std::string &field);

			/* One row's field without moving the cursor, for a list that shows
			 * every row at once. */
			bool GetAt(long row, int field, ControlValue &out) const;
			bool SetAt(long row, int field, const ControlValue &value);

			/* A flat source that tells no one of its changes is asked: observers
			 * hear a moved row when the position or count fields have changed
			 * since the last call, or a table's cells have. True when they had. */
			bool Poll();
			unsigned long TableRevision() const { return table ? revision : 0; }

			virtual int FieldCount() const;
			virtual const DataField &Field(int index) const;
			virtual bool Get(int field, ControlValue &out) const;
			virtual bool Set(int field, const ControlValue &value);
			virtual long Count() const;
			virtual long Position() const;
			virtual bool Seek(long row);

			virtual void DataChanged(DataSource &source, int field);
			virtual void DataSourceDestroyed(DataSource &source);

		private:
			DataSource *flat;
			long first;
			std::vector<DataField> fields;
			long fixedCount;
			int countField, positionField;
			long position;
			long seenPosition, seenCount;
			TableFeed *table;
			int columns;
			unsigned long revision;
			std::vector<double> cells;

			// The flat source's field for a row's field, or -1.
			int FlatField(long row, int field) const;
			// Rows from 0 whose every field exists, up to a limit.
			long Present(long limit) const;

			RowSource(const RowSource &);
			RowSource &operator=(const RowSource &);
		};
	}
}

#endif
