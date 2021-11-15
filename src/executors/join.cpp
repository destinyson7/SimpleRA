#include "global.h"
/**
 * @brief 
 * SYNTAX: R <- JOIN USING <join_algorithm> <relation_name1>, <relation_name2> ON <column_name1> <bin_op> <column_name2> BUFFER <buffer_size>
 */
bool syntacticParseJOIN()
{
    logger.log("syntacticParseJOIN");

    if (tokenizedQuery.size() != 13 || tokenizedQuery[3] != "USING" || tokenizedQuery[7] != "ON" || tokenizedQuery[11] != "BUFFER")
    {
        cout << "SYNTAC ERROR" << endl;
        return false;
    }
    parsedQuery.queryType = JOIN;
    parsedQuery.joinResultRelationName = tokenizedQuery[0];
    parsedQuery.joinFirstRelationName = tokenizedQuery[5];
    parsedQuery.joinSecondRelationName = tokenizedQuery[6];
    parsedQuery.joinFirstColumnName = tokenizedQuery[8];
    parsedQuery.joinSecondColumnName = tokenizedQuery[10];

    string joinAlgorithm = tokenizedQuery[4];
    if (joinAlgorithm == "NESTED")
        parsedQuery.joinAlgorithm = NESTED;
    else if (joinAlgorithm == "PARTHASH")
        parsedQuery.joinAlgorithm = PARTHASH;
    else
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    string binaryOperator = tokenizedQuery[9];
    if (binaryOperator == "==")
        parsedQuery.joinBinaryOperator = EQUAL;
    // else if (binaryOperator == "<")
    //     parsedQuery.joinBinaryOperator = LESS_THAN;
    // else if (binaryOperator == ">")
    //     parsedQuery.joinBinaryOperator = GREATER_THAN;
    // else if (binaryOperator == ">=" || binaryOperator == "=>")
    //     parsedQuery.joinBinaryOperator = GEQ;
    // else if (binaryOperator == "<=" || binaryOperator == "=<")
    //     parsedQuery.joinBinaryOperator = LEQ;
    // else if (binaryOperator == "!=")
    //     parsedQuery.joinBinaryOperator = NOT_EQUAL;
    else
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    regex numeric("[+]?[0-9]+");
    string bufferSize = tokenizedQuery[12];
    if (regex_match(bufferSize, numeric))
    {
        parsedQuery.joinBufferSize = stoi(bufferSize);
    }
    else
    {
        cout << "SYNTAX ERROR" << endl;
        return false;
    }

    return true;
}

bool semanticParseJOIN()
{
    logger.log("semanticParseJOIN");

    if (tableCatalogue.isTable(parsedQuery.joinResultRelationName))
    {
        cout << "SEMANTIC ERROR: Resultant relation already exists" << endl;
        return false;
    }

    if (!tableCatalogue.isTable(parsedQuery.joinFirstRelationName) || !tableCatalogue.isTable(parsedQuery.joinSecondRelationName))
    {
        cout << "SEMANTIC ERROR: Relation doesn't exist" << endl;
        return false;
    }

    if (!tableCatalogue.isColumnFromTable(parsedQuery.joinFirstColumnName, parsedQuery.joinFirstRelationName) || !tableCatalogue.isColumnFromTable(parsedQuery.joinSecondColumnName, parsedQuery.joinSecondRelationName))
    {
        cout << "SEMANTIC ERROR: Column doesn't exist in relation" << endl;
        return false;
    }

    if (parsedQuery.joinBufferSize < 3)
    {
        cout << "SEMANTIC ERROR: Buffer size can't be less than 3" << endl;
        return false;
    }
    return true;
}

void executedNestedJoin()
{
    logger.log("executeNestedJoin");

    Table table1 = *tableCatalogue.getTable(parsedQuery.joinFirstRelationName);
    Table table2 = *tableCatalogue.getTable(parsedQuery.joinSecondRelationName);

    vector<string> columnList = table1.columns;
    columnList.insert(columnList.end(), table2.columns.begin(), table2.columns.end());
	Table *resultantTable = new Table(parsedQuery.joinResultRelationName, columnList);

    Cursor cursor1 = table1.getCursor();
    Cursor cursor2 = table2.getCursor();
	int joinFirstColumnIndex = table1.getColumnIndex(parsedQuery.joinFirstColumnName);
	int joinSecondColumnIndex = table2.getColumnIndex(parsedQuery.joinSecondColumnName);

    while (true)
    {
        unordered_multimap<int, vector<int>> table1Records;
        for(int i = 0; i < parsedQuery.joinBufferSize - 2; i++) {
            vector<int> row = cursor1.getNextRowOfCurPage();
            while (!row.empty())
            {
                table1Records.insert({row[joinFirstColumnIndex], row});
                row = cursor1.getNextRowOfCurPage();
            }
            if (!table1.getNextPage(&cursor1));
                break;
        }
        if (table1Records.empty())
            break;

        while (true)
        {
            vector<int> row = cursor2.getNextRowOfCurPage();
            while (!row.empty())
            {
                auto it = table1Records.equal_range(row[joinSecondColumnIndex]);
                for (auto itr = it.first; itr != it.second; itr++)
                {
                    vector<int> resultantRow = itr->second;
                    resultantRow.insert(resultantRow.end(), row.begin(), row.end());
                    resultantTable->writeRow<int>(resultantRow);
                }
                row = cursor2.getNextRowOfCurPage();
            }
            if (!table2.getNextPage(&cursor2))
                break;
        }
    }

    if(resultantTable->blockify())
        tableCatalogue.insertTable(resultantTable);
    else{
        resultantTable->unload();
        delete resultantTable;
        cout << "Empty Table" << endl;
    }
    return;
}

void executeJOIN()
{
    logger.log("executeJOIN");

    if (parsedQuery.joinAlgorithm == NESTED) 
        executedNestedJoin();
    else
    {
        cout << "No implementation found for part hash join" << endl;
    }
    
    return;
}