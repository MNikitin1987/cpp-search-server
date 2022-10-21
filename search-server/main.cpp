// -------- Начало модульных тестов поисковой системы ----------

// Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
                    "Stop words must be excluded from documents"s);
    }
}

/*
Разместите код остальных тестов здесь
*/

// Поддержка минус-слов. Документы, содержащие минус-слова поискового запроса, не должны включаться в результаты поиска.
void TestExcludeResultsWithMinusWords() {
    SearchServer server;
    server.AddDocument(1, "cat in the city", DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(2, "dog at the city", DocumentStatus::ACTUAL, {2, 3, 4});
    server.AddDocument(3, "carrot in the shop", DocumentStatus::ACTUAL, {3, 4, 5});
    const auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL(found_docs.size(), 1u);
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL(doc0.id, 1);
    ASSERT_HINT(server.FindTopDocuments("cat -the"s).empty(),"This minus word must exclude all results"s);
    ASSERT_EQUAL_HINT(server.FindTopDocuments("city -at"s).size(), 1, "This minus word must exclude all but stay one result"s);
}

// Матчинг документов. При матчинге документа по поисковому запросу должны быть возвращены все слова из поискового запроса, присутствующие в документе. Если есть соответствие хотя бы по одному минус-слову, должен возвращаться пустой список слов.
void TestMatchFunctionOfSearchServer() {
   
    SearchServer server;
    server.AddDocument(5, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    
    // Сначала убеждаемся, что, функция MatchDocument возвращает все слова из поискового запроса
    vector<string> test = {"cat"s, "city"s, "in"s};
    vector<string> test_str;
    DocumentStatus test_stat;
    tie(test_str, test_stat) = server.MatchDocument("cat in city"s, 5);
    ASSERT_HINT(test_str.size() == 3, "Number of words in matched document equal words in request");
    ASSERT_EQUAL(test_str[0], test[0]);
    ASSERT_EQUAL(test_str[1], test[1]);
    ASSERT_EQUAL(test_str[2], test[2]);
    // Затем убеждаемся, что, функция MatchDocument возвращает пустой вектор при наличии в документе минус слова из поискового запроса
    tie(test_str, test_stat) = server.MatchDocument("cat in -city"s, 5);
    ASSERT(test_str.size() == 0);
}

// Сортировка найденных документов по релевантности. Возвращаемые при поиске документов результаты должны быть отсортированы в порядке убывания релевантности.
void TestSortingByRelevationFromAddedDocumentContent() {
    SearchServer server;
    server.AddDocument(2, "cat in the city", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(5, "dog big dog at village", DocumentStatus::ACTUAL, {3,4,3});
    server.AddDocument(7, "dog in city", DocumentStatus::ACTUAL, {3,4,6});
    const auto found_docs = server.FindTopDocuments("dog"s);
    ASSERT_HINT(found_docs.size() == 2, "Number of founded documents euqal test set");
    const Document& doc0 = found_docs[0];
    const Document& doc1 = found_docs[1];
    // Убеждаемся, что порядок документов соответствует их релевантности
    ASSERT_EQUAL(doc0.id, 5);
    ASSERT_EQUAL(doc1.id, 7);
}

// Вычисление рейтинга документов. Рейтинг добавленного документа равен среднему арифметическому оценок документа.
void TestRatingValueFromAddedDocumentContent() {
    SearchServer server;
    server.AddDocument(2, "cat in the city", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(5, "dog big dog at village", DocumentStatus::ACTUAL, {3,4,3});
    server.AddDocument(7, "carrot and dog in city", DocumentStatus::ACTUAL, {-5,4,6,12,33});
    const auto found_docs = server.FindTopDocuments("dog"s);
    ASSERT_HINT(found_docs.size() == 2, "Number of founded documents euqal test set");
    const Document& doc0 = found_docs[0];
    const Document& doc1 = found_docs[1];
    ASSERT_EQUAL_HINT(doc0.rating, 3, "Rating value euqal test set");
    ASSERT_EQUAL_HINT(doc1.rating, 10, "Rating value euqal test set");
}

// Фильтрация результатов поиска с использованием предиката, задаваемого пользователем.
void TestPredicateFunctionOfSearchServer() {
    SearchServer server;
    server.AddDocument(11, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
    server.AddDocument(13, "dog in the city"s, DocumentStatus::ACTUAL, {2, 4, 6});
    server.AddDocument(10, "carrot in the city"s, DocumentStatus::ACTUAL, {3, 5, 7});
    const auto found_docs = server.FindTopDocuments("in"s, [](int document_id, DocumentStatus status, int rating) { (void)status; (void)rating; return document_id % 2 == 0;});
    const Document& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, 10, "Doc ID founded by predicate function euqal test set"s);
}

// Поиск документов, имеющих заданный статус.
void TestDocsWithSpecialDocumentStatus() {
    SearchServer server;
    server.AddDocument(15, "cat in the city", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(13, "dog at village", DocumentStatus::BANNED, {3,4,3});
    server.AddDocument(12, "dog in city", DocumentStatus::BANNED, {3,4,6});
    const auto found_docs0 = server.FindTopDocuments("dog"s, DocumentStatus::BANNED);
    ASSERT_HINT(found_docs0.size() == 2, "Number of founded documents euqal test set");
    const auto found_docs1 = server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL);
    const Document& doc1 = found_docs1[0];
    ASSERT_EQUAL_HINT(doc1.id, 15,  "Doc ID founded with special status euqal test set");
}

// Корректное вычисление релевантности найденных документов.
void TestRelevanceValue() {
    SearchServer server;
    server.AddDocument(15, "cat in the city", DocumentStatus::ACTUAL, {1,2,3});
    server.AddDocument(13, "dog at village", DocumentStatus::ACTUAL, {3,4,3});
    server.AddDocument(12, "dog in city", DocumentStatus::ACTUAL, {3,4,6});
    const auto found_docs = server.FindTopDocuments("dog"s);
    ASSERT_HINT(found_docs.size() == 2, "Number of founded documents euqal test set");
    const Document& doc1 = found_docs[0];
    ASSERT_HINT((abs(doc1.relevance - 0.135155) < 1e-6), "Relevance value euqal test set"s);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // Не забудьте вызывать остальные тесты здесь
	
    RUN_TEST(TestExcludeResultsWithMinusWords);
    RUN_TEST(TestMatchFunctionOfSearchServer);
    RUN_TEST(TestSortingByRelevationFromAddedDocumentContent);
    RUN_TEST(TestRatingValueFromAddedDocumentContent);
    RUN_TEST(TestPredicateFunctionOfSearchServer);
    RUN_TEST(TestDocsWithSpecialDocumentStatus);
    RUN_TEST(TestRelevanceValue);
}

// --------- Окончание модульных тестов поисковой системы -----------