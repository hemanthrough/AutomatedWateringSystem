import java.sql.*;

public class Database {
    private static Connection connection = null;
    private static Statement statement;

    static void openConnection() {
        try {
            String url = "jdbc:sqlite:database.db";
            connection = DriverManager.getConnection(url);
            statement = connection.createStatement();
            System.out.println("Connected to SQLite database.");

            String sqlCreate = "CREATE TABLE IF NOT EXISTS log (id INTEGER NOT NULL, time DATETIME, device_id INTEGER, light INTEGER, temperature INTEGER, humidity INTEGER, moisture INTEGER, score INTEGER, motor INTEGER, corelation DOUBLE, PRIMARY KEY (id));";
            statement.execute(sqlCreate);
        } catch (SQLException e) {
            System.out.println("open db:");
            System.out.println(e.getMessage());
        }
    }

    static void closeConnection() {
        try {
            if (connection != null) {
                connection.close();
            }
        } catch (SQLException ex) {
            System.out.println(ex.getMessage());
        }
    }

    static void insertMeasurement(Measurement measuredObject) {
        try {
            String sql = "INSERT INTO log VALUES (NULL, CURRENT_TIMESTAMP, "+measuredObject.getDevice_id()+", "+measuredObject.getLight()+", "+measuredObject.getTemperature()+", "+measuredObject.getHumidity()+", "+measuredObject.getMoisture()+", "+measuredObject.getScore()+", "+measuredObject.getMotor()+");";
            statement.execute(sql);
        } catch (SQLException e) {
            System.out.println("insertMess db:");
            System.out.println(e.getMessage());
        }
    }

    static void selectTest() {
        try {
            String sql = "SELECT ID, additional FROM log ORDER BY ID desc LIMIT 1;";
            ResultSet rs = statement.executeQuery(sql);

            while(rs.next()) {
                String add = rs.getString("additional");
                Integer id = rs.getInt("ID");

                System.out.println("Saved in database: "+id+" - "+add);
            }
        } catch (SQLException e) {
            System.out.println("selct test db:");
            e.printStackTrace();

        }
    }
}
