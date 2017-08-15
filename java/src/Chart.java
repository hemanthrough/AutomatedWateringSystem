import org.jfree.chart.*;
import org.jfree.chart.axis.*;
import org.jfree.chart.plot.*;
import org.jfree.data.time.*;
import java.awt.*;

class Chart {
    Measurement measurement;
    TimeSeries timeSeriesMoisture;
    TimeSeries timeSeriesTemperature;
    TimeSeries timeSeriesLight;
    ChartPanel chartPanel;

    Chart(Measurement measurementObject) {
        measurement = measurementObject;

        // Define a TimeSeriesChart.
        timeSeriesMoisture = new TimeSeries("Moisture");
        timeSeriesTemperature = new TimeSeries("Temperature");
        timeSeriesLight = new TimeSeries("Light");
        TimeSeriesCollection dataset = new TimeSeriesCollection();
        dataset.addSeries(timeSeriesMoisture);
        dataset.addSeries(timeSeriesTemperature);
        dataset.addSeries(timeSeriesLight);

        JFreeChart chart = ChartFactory.createTimeSeriesChart(
                null,
                "Time",
                "Sensor Data",
                dataset,
                true,
                false,
                false
        );

        // Define a plot and set colors/thicknesses.
        final XYPlot plot = chart.getXYPlot();
        plot.setBackgroundPaint(Color.LIGHT_GRAY);
        plot.getRenderer().setSeriesPaint(0, Color.RED);
        plot.getRenderer().setSeriesPaint(1, Color.GREEN);
        plot.getRenderer().setSeriesPaint(2, Color.BLUE);

        plot.getRenderer().setSeriesStroke(0, new BasicStroke(2.0f));
        plot.getRenderer().setSeriesStroke(1, new BasicStroke(2.0f));
        plot.getRenderer().setSeriesStroke(2, new BasicStroke(2.0f));

        // Set the range of the Y axis.
        NumberAxis rangeAxis = (NumberAxis)plot.getRangeAxis();
        rangeAxis.setRange(0.0, 255.0);

        // Display values of the last two minutes.
        ValueAxis domainAxis = plot.getDomainAxis();
        domainAxis.setAutoRange(true);
        domainAxis.setFixedAutoRange(120000.0);

        chartPanel = new ChartPanel(chart);

        // Disable zooming functionality.
        chartPanel.setDomainZoomable(false);
        chartPanel.setRangeZoomable(false);

        // Disable right-click menu.
        chartPanel.setPopupMenu(null);
    }

    public ChartPanel getChartPanel() {
        return chartPanel;
    }

    private class DataGenerator implements Runnable {
        Measurement measurement;

        DataGenerator(Measurement mes) {
            measurement = mes;
        }
        public void run() {
            while (true) {
                Millisecond millisecond = new Millisecond();

                // Add values to the TimeSeries object.
                timeSeriesMoisture.addOrUpdate(millisecond, measurement.getMoisture());
                timeSeriesTemperature.addOrUpdate(millisecond, measurement.getTemperature());
                timeSeriesLight.addOrUpdate(millisecond, measurement.getLight());

                try {
                    // Wait for 1000 milliseconds.
                    Thread.sleep(1000);
                } catch (InterruptedException ex) {
                    System.out.println(ex.toString());
                }
            }
        }
    }

    void resetChart() {
        timeSeriesMoisture.clear();
        timeSeriesTemperature.clear();
        timeSeriesLight.clear();
    }

    // Starts the chart system.
    void startChart() {
        new Thread(new DataGenerator(this.measurement)).start();
    }
}