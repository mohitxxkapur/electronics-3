function getAllData() {

  var ss = SpreadsheetApp.openById("1Xl7dyNL3prtdf0UCJh7EbTbsW_VgnhDo7nFw0xL_CVk");
  var sheets = ss.getSheets();
  var sheet = ss.getActiveSheet();

  sheet.clear();
  rowHeaders = ["Count", "Humidity", "Precipitation", "Temperature"];
  sheet.appendRow(rowHeaders);

  var firebaseUrl = "https://electronics3-4fcf1-default-rtdb.firebaseio.com/UsersData/KpP4QF3PdbY1AoK3OEuIEsfmk0r1/readings";
  var base = FirebaseApp.getDatabaseByUrl(firebaseUrl);
  var dataSet = [base.getData()];

  // the following lines will depend on the structure of your data
  //

  var rows = [],
      data;

  for (i = 0; i < dataSet.length; i++) {
    data = dataSet[i];
    rows.push([data.count, data.humidity, data.precipitation, data.temperature]);
  }

  dataRange = sheet.getRange(2, 1, rows.length, 4);
  dataRange.setValues(rows);
}