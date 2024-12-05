#include "datalist.h"
#include "checkboxdelegate.h" // CheckBoxDelegate 클래스 포함
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QMetaType>
#include <QNetworkRequest>
#include <QNetworkReply>

DataList::DataList(QTableView *tableView, QObject *parent)
    : tableView(tableView), gridmodel(nullptr), networkManager(new QNetworkAccessManager(this)) {
}

void DataList::GridTableView() {
    QModelIndex index;
    gridmodel = new QStandardItemModel(15, 10, tableView); // 15행 10열 설정
    tableView->setModel(gridmodel);

    // 헤더 설정
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_CHECKBOX, new QStandardItem(QString(" ")));
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_PHOTO, new QStandardItem(QString("Photo")));
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_PATH, new QStandardItem(QString("Path")));

    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_PLATENUM, new QStandardItem(QString("Plate Num")));

    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_START_LOCATION, new QStandardItem(QString("Entry")));
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_START_DATE, new QStandardItem(QString("Entry Date")));
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_END_LOCATION, new QStandardItem(QString("Exit")));
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_END_DATE, new QStandardItem(QString("Exit Date")));

    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_BILL_DATE, new QStandardItem(QString("Bill")));
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_UNPAIDFEE, new QStandardItem(QString("Due")));
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_REGISTRATION, new QStandardItem(QString("Reg")));
    gridmodel->setHorizontalHeaderItem(DataList::Columns::COL_PAYMENT, new QStandardItem(QString("Paid")));

    // 데이터 초기화 및 셀 크기 설정
    for (int row = 0; row < 15; row++) {
        QStandardItem *checkBoxItem = new QStandardItem();
        checkBoxItem->setCheckable(true); // 체크박스 활성화
        checkBoxItem->setCheckState(Qt::Unchecked); // 초기 상태 설정
        // 가운데 정렬
        checkBoxItem->setTextAlignment(Qt::AlignCenter);

        // 아이콘 크기 조정 (스타일 힌트로 체크박스 크기를 조절)
        QFont font = checkBoxItem->font();
        font.setPointSize(12); // 체크박스의 크기를 조정할 때 폰트 크기도 키움
        checkBoxItem->setFont(font);

        gridmodel->setItem(row, DataList::COL_CHECKBOX, checkBoxItem);

        for (int col = 1; col < DataList::Columns::COL_COUNT; col++) {
            index = gridmodel->index(row, col, QModelIndex());
            gridmodel->setData(index, "");
            tableView->setColumnWidth(col, 150);
        }
        tableView->setRowHeight(row, 120);
    }

    // 열 너비 조정
    tableView->setColumnWidth(DataList::Columns::COL_CHECKBOX, 40); // 체크박스 열 너비 설정
    tableView->setColumnWidth(DataList::Columns::COL_PHOTO, 170);
    tableView->setColumnWidth(DataList::Columns::COL_PLATENUM, 80);
    tableView->setColumnWidth(DataList::Columns::COL_START_LOCATION, 50);
    tableView->setColumnWidth(DataList::Columns::COL_END_LOCATION, 50);

    tableView->setColumnWidth(DataList::Columns::COL_START_DATE, 80);
    tableView->setColumnWidth(DataList::Columns::COL_END_DATE, 80);
    tableView->setColumnWidth(DataList::Columns::COL_BILL_DATE, 80);

    tableView->setColumnWidth(DataList::Columns::COL_UNPAIDFEE,100);
    tableView->setColumnWidth(DataList::Columns::COL_REGISTRATION,40);
    tableView->setColumnWidth(DataList::Columns::COL_PAYMENT,40);

    // CheckBoxDelegate 설정
    //CheckBoxDelegate *f_checkboxdelegate = new CheckBoxDelegate(tableView);
    //tableView->setItemDelegateForColumn(0, f_checkboxdelegate);
}

void DataList::populateData(const QList<QList<QVariant>> &data) {
    gridmodel->setRowCount(0); // 기존 데이터 초기화

    for (const QList<QVariant> &row : data) {
        QList<QStandardItem *> items;

        for (int i = 0; i < row.size(); ++i) {
            if (i == DataList::COL_PHOTO) {
                // 이미지 열 처리
                QString imageUrl = row[i].toString(); // 이미지 URL
                QStandardItem* imageItem = new QStandardItem();
                items.append(imageItem);

                // HTTP GET 요청으로 이미지 다운로드
                QNetworkRequest request;
                request.setUrl(QUrl(imageUrl)); // URL 설정
                QNetworkReply* reply = networkManager->get(request); // GET 요청

                // Reply에 행/열 정보를 저장 (이미지 다운로드 후 테이블에 반영하기 위해)
                reply->setProperty("row", gridmodel->rowCount());
                reply->setProperty("column", i); // 해당 열 저장
                connect(reply, &QNetworkReply::finished, this, &DataList::onImageDownloaded);
            }else {
                // 일반 데이터 열 처리
                items.append(new QStandardItem(row[i].toString()));
            }
        }

        gridmodel->appendRow(items);
    }

    for (int row = 0; row < gridmodel->rowCount(); ++row) {
        tableView->setRowHeight(row, 120);
    }
}

void DataList::onImageDownloaded() {
    QNetworkReply* reply = qobject_cast<QNetworkReply*>(sender()); // 신호를 보낸 객체를 가져옴
    if (!reply) {
        qDebug() << "Invalid QNetworkReply";
        return;
    }

    QPixmap pixmap;
    if (reply->error() == QNetworkReply::NoError) {
        // 요청 성공: 다운로드한 이미지 로드
        QByteArray imageData = reply->readAll();
        pixmap.loadFromData(imageData);
    } else {
        // 요청 실패: 기본 이미지 로드
        qDebug() << "Failed to download image:" << reply->errorString();
        if (!pixmap.load(":/images/images/default_car.png")) {
            qDebug() << "Failed to load default image!";
        }
    }

    // 이미지 테이블에 설정
    int row = reply->property("row").toInt();
    int column = reply->property("column").toInt();

    if (!pixmap.isNull() && row >= 0 && column >= 0) {
        // 현재 테이블 뷰의 열 너비 가져오기
        int columnWidth = tableView->columnWidth(column);
        int rowHeight = tableView->rowHeight(row);

        // 이미지 크기를 비율 유지하며 너비에 맞게 스케일링
        pixmap = pixmap.scaled(columnWidth, rowHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QStandardItem* item = gridmodel->item(row, column);
        if (item) {
            item->setData(QVariant(pixmap), Qt::DecorationRole);
        }
    }

    reply->deleteLater();
}
// 작동이안댐..이거해보기
QList<QPair<QString, QString>> DataList::getCheckedClients() const {
    QList<QPair<QString, QString>> checkedClients;

    for (int row = 0; row < gridmodel->rowCount(); ++row) {
        QStandardItem *checkBoxItem = gridmodel->item(row, DataList::COL_CHECKBOX);
        if (checkBoxItem && checkBoxItem->data(Qt::CheckStateRole).toInt() == Qt::Checked) {
            // 번호판과 이메일 정보 가져오기
            QString plateNumber = gridmodel->item(row, DataList::COL_PLATENUM)->text();
            QString email = gridmodel->item(row, DataList::COL_PATH)->text(); // EMAIL 열로 수정 필요
            checkedClients.append(qMakePair(plateNumber, email));
            qDebug() << plateNumber;
        }
    }

    return checkedClients;
}

