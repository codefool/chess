#include "csvtools.h"


CsvRecord Parser::ReadRecord()
{
    std::ifstream myIfStream(this->filename, std::ifstream::in);

    if ( (myIfStream.rdstate() & std::ifstream::failbit ) != 0 )
        errorMsg << "Error opening 'test.csv'." << std::endl;

    std::string csvLine;

    // Read a line from the CSV file
    std::getline(myIfStream, csvLine);

    std::istreambuf_iterator<char> eos;
    std::string theEntireFile(std::istreambuf_iterator<char>(myIfStream), eos);

    // Return a single row from the CSV file
    return this->RecordStringToRecord(csvLine);

}