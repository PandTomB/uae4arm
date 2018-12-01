

class GenericListModel : public gcn::ListModel
{
private:
  std::vector<std::string> values;
  
public:
  GenericListModel()
  {
  }
  
  GenericListModel(const TCHAR *entries[], int count)
  {
    for(int i=0; i<count; ++i)
      values.push_back(entries[i]);
  }

  GenericListModel(std::vector<std::string> list)
  {
    values = list;
  }

  int getNumberOfElements(void)
  {
    return values.size();
  }

  std::string getElementAt(int i)
  {
    if(i < 0 || i >= values.size())
      return "";
    return values[i];
  }

  void clear(void)
  {
    values.clear();
  }
  
  void add(const TCHAR * newvalue)
  {
    values.push_back(newvalue);
  }

  void add(std::string newvalue)
  {
    values.push_back(newvalue);
  }
  
};
